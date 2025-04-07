//
// Created by chic on 2025/4/5.
//

#ifndef ALINUX_DEBUG_INJECT_COMPANION_H
#define ALINUX_DEBUG_INJECT_COMPANION_H

void companion_entry(int fd);
bool check_unix_socket(int fd, bool block) ;
#endif //ALINUX_DEBUG_INJECT_COMPANION_H
