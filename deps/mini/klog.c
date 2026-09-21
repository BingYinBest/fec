/*
 * Minimal klog implementation: writes to stderr instead of /dev/kmsg so the
 * fec tool works unprivileged on Android. Kept standalone (no libcutils).
 */
#include <cutils/klog.h>

#include <stdarg.h>
#include <stdio.h>

void klog_set_level(int level)
{
    (void)level;
}

void klog_write(int level, const char *fmt, ...)
{
    va_list ap;

    (void)level;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

void klog_writev(int level, const struct iovec *iov, int iov_count)
{
    int i;

    (void)level;
    for (i = 0; i < iov_count; i++) {
        fwrite(iov[i].iov_base, 1, iov[i].iov_len, stderr);
    }
    fputc('\n', stderr);
}
