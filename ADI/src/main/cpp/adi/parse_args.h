//
// Created by chic on 2025/5/17.
//


#pragma once


enum {
    OPT_TRACE_EXEC = 1000,
    OPT_TRACE_WAITSOPATH ,
    OPT_TRACE_WAITFUNSYM,
    OPT_INJECT_SOPATH,
    OPT_INJECT_FUNSYM ,
    OPT_INJECT_FUNARG ,
    OPT_MONITORCOUNT,
    OPT_HIDEMAPS,
    OPT_UNLOAD
};

#include <sys/types.h>

 struct ProgramArgs{
    bool help;          // --help 或 -h
    bool verbose;       // --verbose 或 -v
    bool monitor;
    bool inject;
    pid_t pid;
    bool hidemaps;
    bool unload;
    char* injectSoPath;
    char* injectFunSym;
    char* injectFunArg;
    char* waitSoPath;
    char* waitFunSym;
    char* exec;
    char *config;
    unsigned int monitorCount;
     ProgramArgs(){
         help = false;
         verbose = false;
         config = nullptr;
         pid = -1;
         monitor = false;
         inject = false;
         injectSoPath = "";
         injectFunSym = "";
         injectFunArg = "";
         waitSoPath = "";
         waitFunSym = "";
         exec = "";
         config = nullptr;
         monitorCount = 0;
         hidemaps = false;
         unload = false;
     }
} ;


bool parse_args(int argc, char **argv, ProgramArgs *args) ;