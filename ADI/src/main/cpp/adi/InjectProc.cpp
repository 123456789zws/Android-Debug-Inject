//
// Created by chic on 2024/11/19.
//
// system lib
#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <elf.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <cinttypes>
#include "InjectProc.h"
#include <string>
#include <vector>
#include <array>
#include "link.h"
#include "logging.h"
#include "PtraceUtils.h"
#include <link.h>
#include "elf_symbol_resolver.h"
using namespace std;

bool wait_FunSym(pid_t pid, uintptr_t remote_monitor_sym_addr){

    int status;
    struct pt_regs CurrentRegs;
    if (ptrace_getregs(pid, &CurrentRegs) != 0){
        return false;
    }


    uint32_t orig_instr;

    read_proc(pid, remote_monitor_sym_addr, (uintptr_t)&orig_instr, sizeof(orig_instr));
    uint32_t break_addr_instr =  BREAKPOINT_INSTR;
    uint32_t source_addr_instr;
    ptrace_writedata(pid, (uint8_t*) remote_monitor_sym_addr, (uint8_t*)&break_addr_instr, sizeof(break_addr_instr));
    ptrace(PTRACE_CONT, pid, 0, 0);
    int sginal ;
    do{
        wait_for_trace(pid, &status, __WALL);
        if (WIFSTOPPED(status) ) {
            sginal = WSTOPSIG(status);
            if(sginal == SIGTRAP){
                if (ptrace_getregs(pid, &CurrentRegs) != 0) {
                    LOGE("ptrace_getregs failed");
                    return false;
                }
                LOGD("[+][function:%s] cmp pc ",__func__);
                if (static_cast<uintptr_t>(CurrentRegs.pc & ~1) != (remote_monitor_sym_addr & ~1)) {
                    LOGE("stopped at unknown addr %llx", CurrentRegs.pc);
                    ptrace(PTRACE_CONT, pid, NULL, WSTOPSIG(status));
                    continue;
                }
                LOGD("[+][function:%s] reset instr ",__func__);
                ptrace_writedata(pid, (uint8_t*) remote_monitor_sym_addr, (uint8_t*)&orig_instr, sizeof(orig_instr));
                read_proc(pid, remote_monitor_sym_addr, (uintptr_t)&source_addr_instr, sizeof(source_addr_instr));
                if(source_addr_instr != orig_instr){
                    LOGE("bkr reset failed");
                }

                break;
            }
            if (ptrace_getregs(pid, &CurrentRegs) != 0) {
                LOGE("ptrace_getregs failed");
                return false;
            }
            LOGD("[+][function:%s] wait_for_trace sig: %d addr %llx",__func__,sginal,CurrentRegs.pc);
            ptrace(PTRACE_CONT, pid, 0, sginal); // 传递信号
        }
    } while (true);

    return true;
}


uintptr_t wait_lib_load_get_base(pid_t pid, const char *LibPath, vector<MapInfo>& local_maps) {


    uintptr_t ret_libart_load_bias = -1;

    auto remote_map = MapScan(std::to_string(pid));
    auto remote_linker_handle = find_module_return_addr(remote_map, "/apex/com.android.runtime/bin/linker64");

    if(remote_linker_handle == nullptr){
        LOGE("remote_linker_handle is not found \n");
        return false;
    }
    struct pt_regs CurrentRegs;
    // linker nof load self it ,linker 使用符号解析的时候一定要注意,我发现通过hash表和动态段的快速解析方式不好是,只能使用原始读取文件遍历函数的方法算偏移
    auto dl_notify_gdb_of_load_off = reinterpret_cast<uintptr_t>(get_libFile_Symbol_off("/apex/com.android.runtime/bin/linker64", "__dl_notify_gdb_of_load"));
    auto linker64_base_addr =  find_module_base(remote_map,"/apex/com.android.runtime/bin/linker64");
    auto wait_LibPath_base_addr =  find_module_base(remote_map,LibPath);
    if(wait_LibPath_base_addr != nullptr){
        LOGD("wait_LibPath_base_addr : %s is alrealy load",LibPath);
        return reinterpret_cast<uintptr_t>(wait_LibPath_base_addr);
    }
    auto remote_dl_notify_gdb_of_load_addr = dl_notify_gdb_of_load_off + (uintptr_t )linker64_base_addr;
    LOGD("local_dl_notify_gdb_of_load %lx", remote_dl_notify_gdb_of_load_addr);

    uint32_t orig_instr;
    read_proc(pid, remote_dl_notify_gdb_of_load_addr, (uintptr_t)&orig_instr, sizeof(uint32_t));
    link_map linkMap{};
    char libname[100];
    uint32_t break_addr_instr =  BREAKPOINT_INSTR;
    uint32_t source_addr_instr;


    int status;
    do{
        LOGD("Write break");
        ptrace_writedata(pid, (uint8_t*) remote_dl_notify_gdb_of_load_addr, (uint8_t*)&break_addr_instr, sizeof(break_addr_instr));
        ptrace(PTRACE_CONT, pid, 0, 0);
        wait_for_trace(pid, &status, __WALL);
        LOGD("wait_for_trace");
        //出发断点
        if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
            if (ptrace_getregs(pid, &CurrentRegs) != 0) {
                LOGE("ptrace_getregs failed");
                return -1;
            }
            //判断是否在断点位置
            if (static_cast<uintptr_t>(CurrentRegs.pc & ~1) != (remote_dl_notify_gdb_of_load_addr & ~1)) {
                LOGE("stopped at unknown addr %llx", CurrentRegs.pc);
//                auto remote_linker_handle = find_module_return_addr(local_map,"/apex/com.android.runtime/bin/linker64");
//                LOGD("remote_linker_handle addr %llx", remote_linker_handle);
                ptrace(PTRACE_CONT, pid, NULL, WSTOPSIG(status));
                continue;
            }
            LOGD("reset break");
            //恢复断点位置的指令
            ptrace_writedata(pid, (uint8_t*) remote_dl_notify_gdb_of_load_addr, (uint8_t*)&orig_instr, sizeof(orig_instr));
            //读取恢复的断点位置的指令
            read_proc(pid, remote_dl_notify_gdb_of_load_addr, (uintptr_t)&source_addr_instr, sizeof(source_addr_instr));
            //判断写入的是否和原来的指令相同
            if(source_addr_instr != orig_instr){
                LOGD("bkr reset failed");
            }
            //获取第一个参数
            uintptr_t link_map_ptr = CurrentRegs.regs[0];
            read_proc(pid,link_map_ptr,(uintptr_t)&linkMap, sizeof(link_map));
            read_proc(pid,(uintptr_t)linkMap.l_name,(uintptr_t)&libname, 100);
            LOGD("[+]__dl_notify_gdb_of_load:%s",libname);
            if(ends_with(libname,LibPath)){
                ret_libart_load_bias = linkMap.l_addr;
                break;
            }
            ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
            LOGD("ptrace singlestep");
            wait_for_trace(pid, &status, __WALL);
            LOGD("wait_for_trace coutine");


        }
    } while (true);

    return ret_libart_load_bias;
}
bool InjectProc::filter_proce_exec_file(pid_t pid, ContorlProcess &cp)
{
    auto program = get_program(pid);
    LOGD("filter_proce_exec_file: %s ,pid:%d ", program.c_str(),pid);
    for (auto &map: cps) {
        if(program == map.exec){
            cp = map;
            return true;
        }
    }
    return false;
}



bool inject_process(pid_t pid,const char *LibPath,const char *FunctionName,const char*FunctionArgs){


    do{
        auto remote_map = MapScan(std::to_string(pid));
        auto local_map = MapScan(std::to_string(getpid()));
        uintptr_t libc_return_addr = reinterpret_cast<uintptr_t>(find_module_return_addr(remote_map,"libc.so"));
        LOGD("[+][function:%s] libc_return_addr:0x%lx\n",__func__ ,(uintptr_t)libc_return_addr);

        long parameters[6];
        // CurrentRegs 当前寄存器
        // OriginalRegs 保存注入前寄存器
        struct pt_regs CurrentRegs, OriginalRegs;
        if (ptrace_getregs(pid, &CurrentRegs) != 0){
            break;
        }
        // 保存原始寄存器
        memcpy(&OriginalRegs, &CurrentRegs, sizeof(CurrentRegs));

        // 获取mmap函数在远程进程中的地址 以便为libxxx.so分配内存
        // 由于mmap函数在libc.so库中 为了将libxxx.so加载到目标进程中 就需要使用目标进程的mmap函数 所以需要查找到libc.so库在目标进程的起始地址
        void *mmap_addr = find_func_addr(local_map,remote_map,"libc.so","mmap");
        LOGD("[+][function:%s] mmap RemoteFuncAddr:0x%lx\n",__func__ ,(uintptr_t)mmap_addr);

        // mmap映射 <-- 设置mmap的参数
        // void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offsize);
        parameters[0] = NULL; // 设置为NULL表示让系统自动选择分配内存的地址
        parameters[1] = 0x3000; // 映射内存的大小
        parameters[2] = PROT_READ | PROT_WRITE; // 表示映射内存区域 可读|可写|可执行
        parameters[3] = MAP_ANONYMOUS | MAP_PRIVATE; // 建立匿名映射
        parameters[4] = -1; //  若需要映射文件到内存中，则为文件的fd
        parameters[5] = 0; //文件映射偏移量

        // 调用远程进程的mmap函数 建立远程进程的内存映射 在目标进程中为libxxx.so分配内存
        if (ptrace_call(pid, (uintptr_t)mmap_addr, parameters, 6, &CurrentRegs,libc_return_addr) == -1){
            LOGD("[-][function:%s] Call Remote mmap Func Failed, err:%s\n",__func__ , strerror(errno));
            break;
        }
        // 打印一下
        LOGD("[+][function:%s] ptrace_call mmap success, return value=%lX, pc=%lX\n",__func__ , ptrace_getret(&CurrentRegs), ptrace_getpc(&CurrentRegs));

        // 获取mmap函数执行后的返回值，也就是内存映射的起始地址
        // 从寄存器中获取mmap函数的返回值 即申请的内存首地址
        void *RemoteMapMemoryAddr = (void *)ptrace_getret(&CurrentRegs);
        LOGD("[+][function:%s] Remote Process Map Memory Addr:0x%lx\n",__func__ , (uintptr_t)RemoteMapMemoryAddr);

        // 分别获取dlopen、dlsym、dlclose等函数的地址
        void *dlopen_addr, *dlsym_addr, *dlclose_addr, *dlerror_addr;
        dlopen_addr =  find_func_addr(local_map,remote_map,"libdl.so","dlopen");
        dlsym_addr =  find_func_addr(local_map,remote_map,"libdl.so","dlsym");
        dlclose_addr =  find_func_addr(local_map,remote_map,"libdl.so","dlclose");
        dlerror_addr =  find_func_addr(local_map,remote_map,"libdl.so","dlerror");
        // 打印一下
        LOGD("[+][function:%s] Get imports: dlopen: %lx, dlsym: %lx, dlclose: %lx, dlerror: %lx",__func__ , dlopen_addr, dlsym_addr, dlclose_addr, dlerror_addr);

        // 打印注入so的路径
        LOGD("[+][function:%s] LibPath = %s",__func__ , LibPath);

        // 将要加载的so库路径写入到远程进程内存空间中
        /**
         * pid  开始写入数据的地址   写入内容    写入数据大小
         */
        if (ptrace_writedata(pid, (uint8_t *) RemoteMapMemoryAddr, (uint8_t *) LibPath,strlen(LibPath) + 1) == -1) {
            LOGD("[-][function:%s] Write LibPath:%s to RemoteProcess error",__func__ , LibPath);
            break;
        }

        // 设置dlopen的参数,返回值为模块加载的地址
        // void *dlopen(const char *filename, int flag);
        parameters[0] = (uintptr_t) RemoteMapMemoryAddr; // 写入的libPath
        parameters[1] = RTLD_NOW ; // dlopen的标识                            不能使用RTLD_GLOBAL ,会导致无法dlclose 无法关闭so库

        // 执行dlopen 载入so
        if (ptrace_call(pid, (uintptr_t) dlopen_addr, parameters, 2, &CurrentRegs,libc_return_addr) == -1) {
            LOGD("[+][function:%s] Call Remote dlopen Func Failed",__func__ );
            break;
        }

        // RemoteModuleAddr为远程进程加载注入模块的地址
        void *RemoteModuleAddr = (void *) ptrace_getret(&CurrentRegs);
        LOGD("[+][function:%s] ptrace_call dlopen success, Remote Process load module Addr:0x%lx",__func__ ,(long) RemoteModuleAddr);

        // dlopen 错误
        if ((long) RemoteModuleAddr == 0x0){
            LOGD("[-][function:%s] dlopen error",__func__ );
            if (ptrace_call(pid, (uintptr_t) dlerror_addr, parameters, 0, &CurrentRegs,libc_return_addr) == -1) {
                LOGD("[-][function:%s] Call Remote dlerror Func Failed",__func__ );
                break;
            }
            char *Error = (char *) ptrace_getret(&CurrentRegs);
            char LocalErrorInfo[1024] = {0};
            ptrace_readdata(pid, (uint8_t *) Error, (uint8_t *) LocalErrorInfo, 1024);
            LOGD("[-][function:%s] dlopen error:%s",__func__, LocalErrorInfo );
            break;
        }

        LOGD("[+][function:%s] Have func symbols is %s",__func__, FunctionName);
        // 传入了函数的symbols
        // 将so库中需要调用的函数名称写入到远程进程内存空间中
        if (ptrace_writedata(pid, (uint8_t *) RemoteMapMemoryAddr + strlen(LibPath) + 2,(uint8_t *) FunctionName, strlen(FunctionName) + 1) == -1) {
            LOGD("[-][function:%s] Write FunctionName:%s to RemoteProcess error",__func__, FunctionName);
            break;
        }

        // 设置dlsym的参数，返回值为远程进程内函数的地址 调用XXX功能
        // void *dlsym(void *handle, const char *symbol);
        parameters[0] = (uintptr_t) RemoteModuleAddr;
        parameters[1] = (uintptr_t) ((uint8_t *) RemoteMapMemoryAddr + strlen(LibPath) + 2);
        //调用dlsym
        if (ptrace_call(pid, (uintptr_t) dlsym_addr, parameters, 2, &CurrentRegs,libc_return_addr) == -1) {
            LOGD("[-][function:%s] Call Remote dlsym Func Failed",__func__);
            break;
        }
        // RemoteModuleFuncAddr为远程进程空间内获取的函数地址
        void *RemoteModuleFuncAddr = (void *) ptrace_getret(&CurrentRegs);
        if(RemoteModuleFuncAddr == 0){
            LOGD("[-][function:%s] ptrace_call dlsym failed, Remote Process ModuleFunc Addr:0x%lx",__func__,(uintptr_t) RemoteModuleFuncAddr);
        } else{
            LOGD("[+][function:%s] ptrace_call dlsym success, Remote Process ModuleFunc Addr:0x%lx",__func__,(uintptr_t) RemoteModuleFuncAddr);
        }

        int num_arg = 2;

        if (ptrace_writedata(pid, (uint8_t *) RemoteMapMemoryAddr,(uint8_t *) FunctionArgs, strlen(FunctionArgs) + 1) == -1) {
            LOGD("[-][function:%s] Write FunctionArgs:%s to RemoteProcess error",__func__, FunctionName);
            break;
        }
        parameters[1] = (uintptr_t) ((uint8_t *) RemoteMapMemoryAddr);

        LOGD("[+][function:%s] Call Function %s ArgAddr1:0x%lx",__func__,FunctionName,(uintptr_t)parameters[1]);
        if (ptrace_call(pid, (uintptr_t) RemoteModuleFuncAddr, parameters,num_arg ,&CurrentRegs,libc_return_addr) == -1) {
            LOGD("[-][function:%s] Call Remote injected Func Failed",__func__);
            break;
        }

        if (ptrace_setregs(pid, &OriginalRegs) == -1) {
            LOGD("[-][function:%s] Recover reges failed",__func__);
            break;
        }

        LOGD("[+][function:%s] Recover Regs Success",__func__);

        ptrace_getregs(pid, &CurrentRegs);
        if (memcmp(&OriginalRegs, &CurrentRegs, sizeof(CurrentRegs)) != 0) {
            LOGD("[-][function:%s] Set Regs Error",__func__);
        }

    }while(false);

    return true;

}

void InjectProc::monitor_process(pid_t pid){
    ContorlProcess cp;
    if(filter_proce_exec_file(pid,cp)){
        bool stop = stop_int_app_process_entry(pid);
        if(stop){
            auto local_map = MapScan(std::to_string(getpid()));
            uintptr_t  remote_waitSoPath_addr = wait_lib_load_get_base(pid, cp.waitSoPath.c_str(),local_map);
            if(remote_waitSoPath_addr != -1){
                if(!cp.waitFunSym.empty()){

                    uintptr_t remote_waitFunSym_addr = get_libFile_Symbol_off((char*)cp.waitSoPath.c_str(),(char*)cp.waitFunSym.c_str())+remote_waitSoPath_addr;
                    LOGD("waitFunSym is not nul, wait Fun exec,waitFunSymAddr : %lx",remote_waitFunSym_addr);
                    wait_FunSym(pid, (uintptr_t) remote_waitFunSym_addr);
                }
                LOGD("start, inject so to process");
                inject_process(pid,cp.InjectSO.c_str(), cp.InjectFunSym.c_str(),cp.InjectFunArg.c_str());
                LOGD("end,   inject so to process");
            } else{
                LOGE("wait_lib_load_get_base:%s failed",cp.waitSoPath.c_str());

            }
        } else{
            LOGE("stop_int_app_process_entry failed");

        }

        ptrace(PTRACE_CONT, pid, 0, 0);

    }
}


void inject_process_test(int argc, char *argv[]){
    pid_t pid = atoi(argv[1]);

    // attach到目标进程
    if (ptrace_attach(pid) != 0){
        printf("[+][function:%s] ptrace_attach failed\n",__func__ );
        return ;
    }
    inject_process(pid, "/data/local/tmp/libDrmHook.so3","_Z9DrmIdHookPKc","11111111111193baf8cb6a22de8a5ae4bfecc174b1a9405dc71b8b3fac1c734f");

    ptrace_detach(pid, 0);
}

void symbol_find_test(){

//    pid_t pid =845;
//    auto remote_map = MapScan(std::to_string(pid));
//    auto local_map = MapScan(std::to_string(getpid()));
//    void*  libc_base = find_module_base(remote_map,"libart.so");
//    cout<<"libc_base:"<<hex<<libc_base<< endl;  //mmap_addr:0x7f29d23180
//
//    auto mmap_addr3 = get_remote_load_Sym_Addr(libc_base,pid,"_ZN8tinyxml211XMLDocument5ParseEv");
//    auto mmap_addr2 = get_self_load_Sym_Addr("/apex/com.android.art/lib64/libart.so","_ZN8tinyxml211XMLDocument5ParseEv");
//    void * mmap_addr = (void*)mmap;
//    cout<<"mmap_addr:"<<hex<<mmap_addr<< endl;  //mmap_addr:0x7f29d23180
//    cout<<"mmap_addr2:"<<hex<<mmap_addr2<< endl;  //mmap_addr:0x7f29d23180
//    cout<<"mmap_addr3:"<<hex<<mmap_addr3<< endl;  //mmap_addr:0x7f29d23180
}

void func_test(int argc, char *argv[]){

}