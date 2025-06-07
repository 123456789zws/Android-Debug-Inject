//
// Created by chic on 2025/5/31.
//

#include "apatch.h"
#include <linux/fs.h>
#include "logging.h"
#include <sys/stat.h>



uid_t apatch::get_mamager_uid() {
    const char *path = "/data/user_de/0/me.bmax.apatch";
    struct stat file_stat;
    if (stat(path, &file_stat) == 0) {
        // 检查文件的用户 ID 是否与指定的 uid 匹配
        LOGD("get_apatch_mamager_uid:%d",file_stat.st_uid);
        return file_stat.st_uid;
    } else {
        LOGD("stat failed"); // 打印错误信息
        return -1;              // 文件不存在或无法访问
    }
    return -1;
}

bool apatch::uid_granted_root(uid_t uid)
{
    return false;
}

int apatch::getRootFlags(uid_t uid) {
    return 0;
}
bool apatch::uid_should_umount(uid_t uid){

}

