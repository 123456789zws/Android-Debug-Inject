//
// Created by chic on 2025/4/5.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <linux/limits.h>
#include <asm-generic/poll.h>
#include "logging.h"
#include "socket_utils.h"
#include <poll.h>
#include <android/dlext.h>
#include "dl.h"
#include <vector>
#include <thread>

# define LOG_TAG "zygiskCommpanion"

using comp_entry = void(*)(int);

typedef void (*zygisk_companion_entry)(int);

struct companion_module_thread_args {
    int fd;
    zygisk_companion_entry entry;
};

zygisk_companion_entry load_module(int fd) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);

    void *handle = DlopenExt(path, RTLD_NOW);

    if (!handle) return NULL;

    void *entry = dlsym(handle, "zygisk_companion_entry");
    if (!entry) {
        LOGE("Failed to dlsym zygisk_companion_entry: %s\n", dlerror());

        dlclose(handle);

        return NULL;
    }

    return (zygisk_companion_entry)entry;
}



void entry_thread(int client,comp_entry entry){
    struct stat s1;
    fstat(client, &s1);
    entry(client);
    // Only close client if it is the same file so we don't
    // accidentally close a re-used file descriptor.
    // This check is required because the module companion
    // handler could've closed the file descriptor already.
    if (struct stat s2; fstat(client, &s2) == 0) {
        if (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino) {
            close(client);
        }
    }
}


/* WARNING: Dynamic memory based */
void companion_entry(int socket) {
    LOGW("start companion_entry:%d",getpid());

    if (getuid() != 0 || fcntl(socket, F_GETFD) < 0)
        exit(-1);



    // Load modules
    std::vector<comp_entry> modules;
    {
        std::vector<int> module_fds = socket_utils::recv_fds(socket);
        for (int fd : module_fds) {
            comp_entry entry = nullptr;
            struct stat s{};

            if (fstat(fd, &s) == 0 && S_ISREG(s.st_mode)) {
                android_dlextinfo info {
                        .flags = ANDROID_DLEXT_USE_LIBRARY_FD,
                        .library_fd = fd,
                };
                if (void *h = android_dlopen_ext("/jit-cache", RTLD_LAZY, &info)) {
                    *(void **) &entry = dlsym(h, "zygisk_companion_entry");
                } else {
                    LOGW("Failed to dlopen zygisk module: %s\n", dlerror());
                }
            }
            modules.push_back(entry);
            close(fd);
        }
    }

    // ack
    socket_utils::write_u32(socket, 0);

    // Start accepting requests
    pollfd pfd = { socket, POLLIN, 0 };
    for (;;) {
        poll(&pfd, 1, -1);
        if (pfd.revents && !(pfd.revents & POLLIN)) {
            // Something bad happened in magiskd, terminate zygiskd
            exit(0);
        }
        int client = socket_utils::recv_fd(socket);
        if (client < 0) {
            // Something bad happened in magiskd, terminate zygiskd
            exit(0);
        }
        int module_id = socket_utils::read_u32(client);

        if (module_id >= 0 && module_id < modules.size() && modules[module_id]) {
            std::thread new_thread(entry_thread,client,modules[module_id]);
            new_thread.detach(); // 线程分离，主线程不等待
        } else {
            close(client);
        }
    }

}