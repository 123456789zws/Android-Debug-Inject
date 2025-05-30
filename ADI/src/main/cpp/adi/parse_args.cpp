//
// Created by chic on 2025/5/17.
//

#include "parse_args.h"
#include <getopt.h>  // 必须包含此头文件
#include <cstring>
#include <cstdio>
#include <dirent.h>
#include <cstdlib>
#include "logging.h"

/**
 * @brief Get the pid by pkg_name
 * 成功返回 true
 * 失败返回 false
 *
 * @param pid
 * @param task_name
 * @return true
 * @return false
 */
bool get_pid_by_name(pid_t *pid, char *task_name){
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[150];
    char cur_task_name[1024];
    char buf[1024];

    dir = opendir("/proc");
    if (NULL != dir){
        while ((ptr = readdir(dir)) != NULL){ //循环读取/proc下的每一个文件/文件夹
            //如果读取到的是"."或者".."则跳过，读取到的不是文件夹名字也跳过
            if ((strcmp(ptr->d_name, ".") == 0) || (strcmp(ptr->d_name, "..") == 0))
                continue;
            if (DT_DIR != ptr->d_type)
                continue;

            sprintf(filepath, "/proc/%s/cmdline", ptr->d_name); //生成要读取的文件的路径
            fp = fopen(filepath, "r");
            if (NULL != fp){
                if (fgets(buf, 1024 - 1, fp) == NULL){
                    fclose(fp);
                    continue;
                }
                sscanf(buf, "%s", cur_task_name);
                //如果文件内容满足要求则打印路径的名字（即进程的PID）
                if (strstr(task_name, cur_task_name)){
                    *pid = atoi(ptr->d_name);
                    return true;
                }
                fclose(fp);
            }
        }
        closedir(dir);
    }
    return false;
}



bool parse_args(int argc, char **argv, ProgramArgs *args) {
    // 初始化默认值
    int opt;
    int option_index = 0;
    char* niceName= nullptr;
    int is_niceName = -1;
    bool is_config = false;
    int is_inject_sopath = -1;
    int is_inject_funsym = -1;
    // 定义长选项（long options）
    static struct option long_options[] = {
            {"help",    no_argument,       0, 'h'},
            {"monitor",    no_argument,       0, 'm'},
            {"inject",    no_argument,       0, 'i'},
            {"config",    required_argument,       0, 'c'},
            {"verbose", no_argument,       0, 'v'},
            {"pid",  required_argument, 0, 'p'},
            {"niceName",  required_argument, 0, 'n'},
            {"exec",   required_argument, 0, OPT_TRACE_EXEC},
            {"waitSoPath",   required_argument, 0, OPT_TRACE_WAITSOPATH},
            {"waitFunSym",   required_argument, 0, OPT_TRACE_WAITFUNSYM},
            {"injectSoPath",  required_argument, 0, OPT_INJECT_SOPATH},
            {"injectFunSym",   required_argument, 0, OPT_INJECT_FUNSYM},
            {"injectFunArg",   required_argument, 0, OPT_INJECT_FUNARG},
            {"monitorCount",   required_argument, 0,OPT_MONITORCOUNT},
            {"hidemaps",   required_argument, 0,OPT_HIDEMAPS},
            {"unload",   required_argument, 0,OPT_UNLOAD},
            {0, 0, 0, 0}  // 结束标记
    };

    // 定义短选项（short options）
    const char *short_options = "himvc:p:n:";


    // 解析选项
    while ((opt = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (opt) {
            case 'c':
                args->config = strdup(optarg);
                is_config = true;
                break;
            case 'h':
                args->help = true;
                break;
            case 'i':
                args->inject = true;
                break;
            case 'n':{
                        niceName = strdup(optarg);
                        int tmp_pid = -1;
                        if (get_pid_by_name(&tmp_pid, niceName)){
                            args->pid = tmp_pid;
                        } else{
                            LOGE("get_pid_by_name  failed");
                            return false;
                        }
                    }
                break;
            case 'p':
                args->pid = atoi(optarg);
                break;
            case 'm':
                args->monitor = true;
                break;
            case 'v':
                args->verbose = true;
                break;

            case OPT_TRACE_EXEC:
                args->exec = strdup(optarg);
                break;
            case OPT_TRACE_WAITSOPATH:
                args->waitSoPath = strdup(optarg);
                break;
            case OPT_TRACE_WAITFUNSYM:
                args->waitFunSym = strdup(optarg);
                break;
            case OPT_INJECT_SOPATH:
                args->injectSoPath = strdup(optarg);
                break;
            case OPT_INJECT_FUNSYM:
                args->injectFunSym = strdup(optarg);
                break;
            case OPT_INJECT_FUNARG:
                args->injectFunArg = strdup(optarg);
                break;
            case OPT_MONITORCOUNT:
                args->monitorCount = atoi(optarg);
                break;
            case OPT_HIDEMAPS:
                args->hidemaps = true;
                break;
            case OPT_UNLOAD:
                args->unload = true;
                break;

        }
    }

    if (args->monitor == args->inject) {
        LOGE("--monitor or --inject arg error");
        return false;

    }
    if(is_config){
        return true;
    }
    if(args->pid == -1){
        LOGE("error,pid is -1");
        return false;
    }


    return true;
}