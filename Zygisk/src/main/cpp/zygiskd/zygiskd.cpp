//
// Created by chic on 2024/12/4.
//

#include "zygiskd.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>          /* See NOTES */
#include <vector>
#include <linux/in.h>
#include "socket_utils.h"
#include "RootImp.h"
#include <thread>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>
#include "companion.h"
#include "misc.hpp"
#include "vector"
#define EPOLL_SIZE 10



void Zygiskd::foreach_module(int fd,dirent *entry,int modfd){
    if (faccessat(modfd, "disable", F_OK, 0) == 0) {
        return;
    }
    Module info;
    LOGD("enable_module:%s",entry->d_name);
    if (faccessat(modfd, "zygisk/armeabi-v7a.so", F_OK, 0) != 0) {
        return;
    }
    info.z32 = openat(modfd, "zygisk/armeabi-v7a.so", O_RDONLY | O_CLOEXEC);
    if (faccessat(modfd, "zygisk/arm64-v8a.so", F_OK, 0) != 0) {
        return;
    }
    info.z64 = openat(modfd, "zygisk/arm64-v8a.so", O_RDONLY | O_CLOEXEC);
    info.name = entry->d_name;

    module_list.push_back(info);

}

void Zygiskd::collect_modules(){
    moduleRoot = MODULEROOT;
    auto dir = opendir(MODULEROOT);
    if (!dir)
        return;
    int dfd = dirfd(dir);
    for (dirent *entry; (entry = readdir(dir));) {
        if (entry->d_type == DT_DIR && (0 != strcmp(entry->d_name, "."))&& (0 != strcmp(entry->d_name, ".."))) {
            int modfd = openat(dfd, entry->d_name, O_RDONLY | O_CLOEXEC);
            foreach_module(dfd, entry, modfd);
            close(modfd);
        }
    }
}


// The following code runs in magiskd

static std::vector<int> get_module_fds(bool is_64_bit) {
    std::vector<int> fds;
    // All fds passed to send_fds have to be valid file descriptors.
    // To workaround this issue, send over STDOUT_FILENO as an indicator of an
    // invalid fd as it will always be /dev/null in magiskd
    auto module_list = Zygiskd::getInstance().getModule_list();
#if defined(__LP64__)
    if (is_64_bit) {
        std::transform(module_list.begin(), module_list.end(), std::back_inserter(fds),
                       [](const Module &info) { return info.z64 < 0 ? STDOUT_FILENO : info.z64; });
    } else
#endif
    {
        std::transform(module_list.begin(), module_list.end(), std::back_inserter(fds),
                       [](const Module &info) { return info.z32 < 0 ? STDOUT_FILENO : info.z32; });
    }
    return fds;
}


static pthread_mutex_t zygiskd_lock = PTHREAD_MUTEX_INITIALIZER;
static int zygiskd_sockets[] = { -1, -1 };
#define zygiskd_socket zygiskd_sockets[is_64_bit]

static void connect_companion(int client, bool is_64_bit) {
    mutex_guard g(zygiskd_lock);

    if (zygiskd_socket >= 0) {
        // Make sure the socket is still valid
        pollfd pfd = { zygiskd_socket, 0, 0 };
        poll(&pfd, 1, 0);
        if (pfd.revents) {
            // Any revent means error
            close(zygiskd_socket);
            zygiskd_socket = -1;
        }
    }
    if (zygiskd_socket < 0) {
        int fds[2];
        socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds);
        zygiskd_socket = fds[0];
        if (fork_dont_care() == 0) {
#if defined(__LP64__)
            std::string exe = Zygiskd::getInstance().get_exec_path();
#else
            std::string exe = Zygiskd::getInstance().get_exec_path()+"32";
#endif
            // This fd has to survive exec
            fcntl(fds[1], F_SETFD, 0);
            char buf[16];
            ssprintf(buf, sizeof(buf), "%d", fds[1]);
            execl(exe.c_str(), "","companion", buf, (char *) nullptr);
            exit(-1);
        }
        close(fds[1]);
        std::vector<int> module_fds = get_module_fds(is_64_bit);
        socket_utils::send_fds(zygiskd_socket, module_fds.data(), module_fds.size());
        // Wait for ack
        if (socket_utils::read_u32(zygiskd_socket) != 0) {
            LOGE("zygiskd startup error\n");
            return;
        }
    }
    socket_utils::send_fd(zygiskd_socket, client);
}



void handle_daemon_action(int cmd,int fd) {
    LOGD("handle_daemon_action");

    switch (cmd) {
        case (uint8_t)zygiskComm::SocketAction::RequestLogcatFd:{
            LOGD("RequestLogcatFd");
            while (1){
                std::string msg =socket_utils::read_string(fd);
            }
            break;
        }

        case (uint8_t)zygiskComm::SocketAction::GetProcessFlags:{
            LOGD("GetProcessFlags");
            uid_t uid =socket_utils::read_u32(fd);
            int flags =  RootImp::getInstance().getProcessFlags(uid);
            socket_utils::write_u32(fd,flags);
            break;
        }
        case (uint8_t)zygiskComm::SocketAction::ReadModules:
            LOGD("ReadModules");
            zygiskComm::WriteModules(fd,Zygiskd::getInstance().getModule_list());
            break;
        case (uint8_t)zygiskComm::SocketAction::RequestCompanionSocket:{
            LOGD("RequestCompanionSocket");
            uint32_t arch = socket_utils::read_u32(fd);
            connect_companion(fd,arch);
            break;
        }
        case (uint8_t)zygiskComm::SocketAction::GetModuleDir:
            LOGD("GetModuleDir");
            size_t index = socket_utils::read_usize(fd);
            auto moduleDir = Zygiskd::getInstance().getModul_by_index(index);
            int send_fd = open(moduleDir.c_str(),O_RDONLY);
            socket_utils::send_fd(fd,send_fd);
            break;
    }

    close(fd);
}

void zygiskd_handle(int client_fd){
    uint8_t cmd =  socket_utils::read_u8(client_fd);
    switch (cmd) {
        case (uint8_t)zygiskComm::SocketAction::PingHeartBeat:
            LOGD("HandleEvent PingHeartBeat");
            break;
        case (uint8_t)zygiskComm::SocketAction::ZygoteRestart:
            LOGD("HandleEvent ZygoteRestart");
            break;

        case (uint8_t)zygiskComm::SocketAction::SystemServerStarted:
            LOGD("HandleEvent SystemServerStarted");
            break;
        default:
            LOGD("HandleEvent default");
            int new_fd = dup(client_fd);
            std::thread ZygiskThread(handle_daemon_action,cmd, new_fd);
            ZygiskThread.detach();
    }
}

void zygiskd_main(char * exec_path ,const char* requestSocketPath){

    int server_fd;
    int client_fd;
    int epfd;

    zygiskComm::InitRequestorSocket(requestSocketPath);
    Zygiskd::getInstance().set_exec_path(exec_path);
    Zygiskd::getInstance().collect_modules();
    server_fd = socket(AF_UNIX,SOCK_STREAM,0);

    if (server_fd < 0){
        LOGE("Create Socket Errors, %d",server_fd);
        exit(server_fd);
    }
    struct sockaddr_un addr;
    zygiskComm::set_sockaddr(addr);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOGE("bind error");
        return ;
    }

    if (listen(server_fd, 10)<0)
    {
        LOGE("listen error");
        return ;
    }

    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0)
    {
        LOGE("Epoll Create");
        exit(-1);
    }

    struct epoll_event ev;
    struct epoll_event events[EPOLL_SIZE];
    ev.data.fd = server_fd;
    ev.events = EPOLLIN ;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFD, 0)| O_NONBLOCK);

    while (1)
    {
        int nfds = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (nfds == -1)
        {
            if(errno != EINTR)
                LOGE("epoll failed");
            continue;
        }
        for (int i=0;i < nfds;i++)
        {
            if (events[i].data.fd==server_fd&&(events[i].events & EPOLLIN))
            {
                struct sockaddr_in remote_addr;

                socklen_t socklen = sizeof(struct sockaddr_in);
                client_fd = accept(server_fd, (struct sockaddr *)&remote_addr, &socklen);
                if (client_fd > 0)
                {
                    zygiskd_handle(client_fd);
                }
                close(client_fd);
            }else{
                LOGE("unknow  error else");
            }
        }
    }
    close(server_fd);
    close(epfd);
    return ;
}


int main(int argc, char *argv[]) {

    if (argc > 1) {
        if (strcmp(argv[1], "companion") == 0) {
            if (argc < 3) {
                LOGI("Usage: zygiskd companion <fd>\n");
                return 1;
            }
            int fd = atoi(argv[2]);
            companion_entry(fd);
            return 0;
        }

        else if (strcmp(argv[1], "version") == 0) {

            return 0;
        }

        else if (strcmp(argv[1], "root") == 0) {
//            root_impls_setup();
//
//            struct root_impl impl;
//            get_impl(&impl);
//
//            char impl_name[LONGEST_ROOT_IMPL_NAME];
//            stringify_root_impl_name(impl, impl_name);
//
//            LOGI("Root implementation: %s\n", impl_name);
            return 0;
        }
        else if (strcmp(argv[1], "unix_socket") == 0) {
            LOGD("zygiskd start run , arg1:%s",argv[1]);
            zygiskd_main(argv[0],argv[2]);
            return 0;
        }
        else {
            LOGI("Usage: zygiskd [companion|version|root|unix_socket]\n");
            return 0;
        }
    }


    return 0;
}