//
// Created by chic on 2025/5/31.
//

#pragma once

#include "RootImp.h"

class magisk: public RootImp {

    uid_t get_mamager_uid();
    bool uid_granted_root(uid_t uid);
    int getRootFlags(uid_t uid);
    bool uid_should_umount(uid_t uid);

};