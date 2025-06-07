//
// Created by chic on 2025/5/31.
//

#include "magisk.h"
#include <linux/fs.h>
#include "logging.h"
#include <sys/stat.h>
#include <stdio.h>

uid_t magisk::get_mamager_uid() {

    const char *path = "/data/user_de/0/com.topjohnwu.magisk";
    struct stat file_stat;
    if (stat(path, &file_stat) == 0) {
        // 检查文件的用户 ID 是否与指定的 uid 匹配
        LOGD("get_magisk_mamager_uid:%d",file_stat.st_uid);
        return file_stat.st_uid;
    } else {
        LOGD("stat failed"); // 打印错误信息
        return -1;              // 文件不存在或无法访问
    }

    return -1;
}

bool magisk::uid_granted_root(uid_t uid){
    char sqlite_cmd[256];
    snprintf(sqlite_cmd, sizeof(sqlite_cmd), "select 1 from policies where uid=%d and policy=2 limit 1", uid);

    char *const argv[] = { "magisk", "--sqlite", sqlite_cmd, NULL };

    char result[32];
//    if (!exec_command(result, sizeof(result), (const char *)"", argv)) {
//        LOGE("Failed to execute magisk binary: %s\n", strerror(errno));
//        errno = 0;
//
//        return false;
//    }

    return result[0] != '\0';
}

int magisk::getRootFlags(uid_t uid) {
    int version=0;

    if (is_command_available("magisk")) {
        FILE *fp = popen("magisk -V", "r");
        if (fp == NULL) {
            return 0;  // 如果打开管道失败，返回 0
        }
        char result[256];
        if (fgets(result, sizeof(result), fp) != NULL) {
            // 如果读取到结果，表示命令存在
            fclose(fp);
            version = atoi(result);
        } else {
            // 如果没有读取到结果，表示命令不存在
            fclose(fp);
            return 0;
        }
    } else {
        printf("Magisk command is not available.\n");
        return 0;
    }
    if(version <= 0){
        return 0;
    } else {
        return PROCESS_ROOT_IS_MAGISK;

    }

    return 0;
}

bool magisk::uid_should_umount(uid_t uid) {

    return false;
}
