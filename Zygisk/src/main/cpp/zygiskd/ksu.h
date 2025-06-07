//
// Created by chic on 2025/5/31.
//

#pragma once

#include "RootImp.h"


// 常量定义
#define KERNEL_SU_OPTION 0xdeadbeefu
#define CMD_GET_VERSION 2

// 假定的最小和最大版本号，请根据实际需要设置
#define MIN_KSU_VERSION 10
#define MAX_KSU_VERSION 20
#define CMD_UID_GRANTED_ROOT 12
#define CMD_UID_SHOULD_UMOUNT 13

class ksu : public RootImp {

public:
    uid_t get_mamager_uid() ;
    bool uid_granted_root(uid_t uid);
    int getRootFlags(uid_t uid);
    bool uid_should_umount(uid_t uid);
};


