//
// Created by chic on 2025/5/31.
//

#include "ksu.h"
#include <linux/fs.h>
#include "logging.h"
#include <sys/stat.h>
#include <sys/prctl.h>

uid_t ksu::get_mamager_uid() {

    const char *path = "/data/user_de/0/me.weishu.kernelsu";
    struct stat file_stat;
    if (stat(path, &file_stat) == 0) {
        // 检查文件的用户 ID 是否与指定的 uid 匹配
        LOGD("get_ksu_mamager_uid:%d",file_stat.st_uid);
        return file_stat.st_uid;
    } else {
        LOGE("stat failed"); // 打印错误信息
        return -1;              // 文件不存在或无法访问
    }

    return -1;
}

bool ksu::uid_granted_root(uid_t uid) {

    unsigned int result = 0;
    bool granted = false;

    // 调用 prctl
    int ret = prctl(
            KERNEL_SU_OPTION,
            CMD_UID_GRANTED_ROOT,
            uid,
            (long)&granted, // 强制转换为 long 类型以适配 prctl 参数
            (long)&result   // 强制转换为 long 类型以适配 prctl 参数
    );

    if (ret < 0) {
        LOGE("prctl failed");
        return false;
    }

    if (result != KERNEL_SU_OPTION) {
        LOGE("uid_granted_root failed");
        return false;
    }

    return granted != 0;

}

int ksu::getRootFlags(uid_t uid) {


    int version = 0;
    if(prctl(KERNEL_SU_OPTION,CMD_GET_VERSION,&version,0,0)<0){
        return 0;
    }
    // 判断版本号
    const int MAX_OLD_VERSION = MIN_KSU_VERSION - 1;
    if (version == 0) {
        return 0;
    } else if (version >= MIN_KSU_VERSION && version <= MAX_KSU_VERSION) {
        return PROCESS_ROOT_IS_KSU;
    } else if (version >= 1 && version <= MAX_OLD_VERSION) {
        return 0;
    } else {
        return 0;
    }
}

bool ksu::uid_should_umount(uid_t uid) {
    uint32_t result = 0;
    bool umount = false;
    prctl(KERNEL_SU_OPTION, CMD_UID_SHOULD_UMOUNT, uid, &umount, &result);

    if ((int)result != KERNEL_SU_OPTION) return false;

    return umount;
}
