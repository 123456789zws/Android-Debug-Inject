#include "daemon.h"
#include "logging.h"
#include "zygisk.hpp"
#include "module.hpp"
#include <dlfcn.h>
#include "clean.h"
using namespace std;
void *self_handle = nullptr;


extern "C" [[gnu::visibility("default")]]
void entry(void* handle, const char* path) {
    self_handle = handle;
    zygiskComm::InitRequestorSocket(path);
    if (!zygiskComm::PingHeartbeat()) {
        LOGE("Zygisk daemon is not running");
        return;
    }

//#ifdef NDEBUG
//    logging::setfd(zygiskd::RequestLogcatFd());
//#endif

    LOGD("Start hooking");

    Dl_info dl_info;
    dladdr((void*)hook_entry, reinterpret_cast<Dl_info *>(&dl_info));
    string file_path = dl_info.dli_fname;
    void* so_start_addr =dl_info.dli_fbase;
    LOGD("hook_entry %s addr %p",file_path.c_str(),so_start_addr);
    size_t so_size = remove_soinfo(file_path.c_str(), 1, 0, false);
    LOGD("hook_entry so_size %zu ",so_size);

    hook_entry(so_start_addr,so_size);
}
