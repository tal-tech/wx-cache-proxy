#ifndef __LOG_ETCD_H
#define __LOG_ETCD_H

struct logger_etcd {
    char *name;  /* log file name */
    int  level;  /* log level */
    int  fd;     /* log file descriptor */
    int  nerror; /* # log error */
};

#define loga_etcd(...) do {                                                      \
    _log(__FILE__, __LINE__, 0, __VA_ARGS__);                               \
} while (0)

#define nc_etcd_write(_d, _b, _n)    \
    write(_d, _b, (size_t)(_n))

static struct logger_etcd logger_etcd;

#define LOG_EMERG_ETCD   0
#define LOG_PVERB_ETCD   11
#define LOG_MAX_LEN_ETCD 256




int log_init_etcd(int level, char *name);
void log_deinit_etcd(void);
int _vscnprintf_etcd(char *buf, size_t size, const char *fmt, va_list args);
int _scnprintf_etcd(char *buf, size_t size, const char *fmt, ...);
int _vscnprintf_etcd(char *buf, size_t size, const char *fmt, va_list args);
void _xes_log(const char *file, int line, int panic, const char *fmt, ...);

#define nc_strftime_etcd(_s, _n, fmt, tm)        \
    (int)strftime((char *)(_s), (size_t)(_n), fmt, tm)

#define nc_scnprintf_etcd(_s, _n, ...)       \
    _scnprintf_etcd((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define nc_vscnprintf_etcd(_s, _n, _f, _a)   \
    _vscnprintf_etcd((char *)(_s), (size_t)(_n), _f, _a)

#define xes_log(...) do {                                                      \
    _xes_log(__FILE__, __LINE__, 0, __VA_ARGS__);                               \
} while (0)

#endif