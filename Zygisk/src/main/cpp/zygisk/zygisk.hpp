#pragma once

#include <jni.h>
#include <sys/types.h>

extern void *self_handle;

void hook_entry(void *start_addr, size_t block_size);

void hookJniNativeMethods(JNIEnv *env, const char *clz, JNINativeMethod *methods, int numMethods);
