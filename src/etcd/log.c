/*
 * twemproxy - A fast and lightweight proxy for memcached protocol.
 * Copyright (C) 2011 Twitter, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "log.h"

int
log_init_etcd(int level, char *name)
{
    struct logger_etcd *l = &logger_etcd;

    l->level = level;
    l->name = name;
    if (name == NULL || !strlen(name)) {
        l->fd = STDERR_FILENO;
    } else {
        l->fd = open(name, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (l->fd < 0) {
            printf("opening log file '%s' failed: %s", name,
                       strerror(errno));
            return -1;
        }
    }

    return 0;
}

void
log_deinit_etcd(void)
{
    struct logger_etcd *l = &logger_etcd;

    if (l->fd < 0 || l->fd == STDERR_FILENO) {
        return;
    }

    close(l->fd);
}


int _vscnprintf_etcd(char *buf, size_t size, const char *fmt, va_list args) {
    int n;

    n = vsnprintf(buf, size, fmt, args);

    /*
     * The return value is the number of characters which would be written
     * into buf not including the trailing '\0'. If size is == 0 the
     * function returns 0.
     *
     * On error, the function also returns 0. This is to allow idiom such
     * as len += _vscnprintf(...)
     *
     * See: http://lwn.net/Articles/69419/
     */
    if (n <= 0) {
        return 0;
    }

    if (n < (int) size) {
        return n;
    }

    return (int) (size - 1);
}


int _scnprintf_etcd(char *buf, size_t size, const char *fmt, ...) {
    va_list args;
    int n;

    va_start(args, fmt);
    n = _vscnprintf_etcd(buf, size, fmt, args);
    va_end(args);

    return n;
}

void
_xes_log(const char *file, int line, int panic, const char *fmt, ...)
{
    struct logger_etcd *l = &logger_etcd;
    int len, size, errno_save;
    char buf[LOG_MAX_LEN_ETCD];
    va_list args;
    ssize_t n;
    struct timeval tv;

    if (l->fd < 0) {
        return;
    }

    errno_save = errno;
    len = 0;            /* length of output buffer */
    size = LOG_MAX_LEN_ETCD; /* size of output buffer */

    gettimeofday(&tv, NULL);
    buf[len++] = '[';
    len += nc_strftime_etcd(buf + len, size - len, "%Y-%m-%d %H:%M:%S.", localtime(&tv.tv_sec));
    len += nc_scnprintf_etcd(buf + len, size - len, "%03ld", tv.tv_usec/1000);
    len += nc_scnprintf_etcd(buf + len, size - len, "] %s:%d ", file, line);

    va_start(args, fmt);
    len += nc_vscnprintf_etcd(buf + len, size - len, fmt, args);
    va_end(args);

    buf[len++] = '\n';

    n = nc_etcd_write(l->fd, buf, len);
    if (n < 0) {
        l->nerror++;
    }

    errno = errno_save;

    if (panic) {
        abort();
    }
}