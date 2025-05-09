//
// Created by chic on 2024/11/19.
//

#ifndef RXPOSED_INJECTPROC_H
#define RXPOSED_INJECTPROC_H
#include <sys/types.h>
#include <string>
#include <set>
#include <vector>
#define STOPPED_WITH(status,sig, event) WIFSTOPPED(status) && (status >> 8 == ((sig) | (event << 8)))
void func_test(int argc, char *argv[]);


class ContorlProcess {
public:

    std::string exec;
    std::string waitSoPath;
    std::string waitFunSym;
    std::string InjectSO;
    std::string InjectFunSym;
    std::string InjectFunArg;
    unsigned int monitorCount;

};


class InjectProc {

public:


    void setTracePid(pid_t pid){
        traced_pid = pid;
    }

    std::set<pid_t>& get_Tracee_Process(){
        return monitor_pid;
    }
    pid_t getTracePid(){
        return traced_pid;
    }

    void monitor_process(pid_t pid);

    bool filter_proce_exec_file(pid_t pid, ContorlProcess &cp);

    void add_childProces(ContorlProcess cp){
        cps.emplace_back(cp);
    }

    // 获取单例实例的静态方法
    static InjectProc& getInstance() {
        static InjectProc instance; // 使用static保证只创建一次
        return instance;
    }

private:
    InjectProc(){

    }
    std::vector<ContorlProcess> cps;
    std::string requestoSocket;
    pid_t traced_pid;
    std::string zygote64_Inject_So;
    std::string zygote32_Inject_So;
    std::set<pid_t> monitor_pid;

};


#endif //RXPOSED_INJECTPROC_H
