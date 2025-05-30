//
// Created by chic on 2025/5/14.
//

#ifndef ANDROID_DEBUG_INJECT_CLEAN_H
#define ANDROID_DEBUG_INJECT_CLEAN_H



size_t  remove_soinfo(const char *path, size_t load, size_t unload, bool spoof_maps);
void reSoMap(const char *path);
#endif //ANDROID_DEBUG_INJECT_CLEAN_H
