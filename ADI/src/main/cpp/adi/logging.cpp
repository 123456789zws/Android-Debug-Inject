#include <unistd.h>
#include <stdio.h>
#include "logging.h"

namespace logging {
    static int logfd = -1;

    void setfd(int fd) {
        close(logfd);
        logfd = fd;
    }

    int getfd() {
        return logfd;
    }

    void log(int prio, const char* tag, const char* fmt, ...) {
        if (logfd == -1) {
            va_list ap;
            va_start(ap, fmt);
            __android_log_vprint(prio, tag, fmt, ap);
            va_end(ap);
        } else {
            char buf[4096];
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(buf, sizeof(buf), fmt, ap);
            va_end(ap);

        }
    }
}
