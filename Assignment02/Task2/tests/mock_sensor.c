#define _POSIX_C_SOURCE 200809L

#include "sms_sensor.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdatomic.h>

/*
 * Test-only mock sensor for x86 Valgrind testing.
 * Does not replace the real ARM kernel driver.
 */

static atomic_uint sample_count = 0;

extern int __real_open(const char *path, int flags, ...);

int __wrap_open(const char *path, int flags, ...)
{
    if (strcmp(path, "/dev/sms_sensor") != 0) {
        return __real_open(path, flags, 0600);
    }

    int pipefd[2];

    if (pipe(pipefd) != 0) {
        return -1;
    }

    unsigned int count = atomic_fetch_add(
        &sample_count, 1
    );

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long long timestamp =
        (unsigned long long)now.tv_sec * 1000ULL +
        (unsigned long long)now.tv_nsec / 1000000ULL;

    double temperature = 15.0 + (count % 30);
    double humidity = 30.0 + ((count * 7) % 61);

    char buffer[128];

    int length = snprintf(
        buffer,
        sizeof(buffer),
        "%llu,%.2f,%.2f\n",
        timestamp,
        temperature,
        humidity
    );

    if (length < 0 || length >= (int)sizeof(buffer)) {
        close(pipefd[0]);
        close(pipefd[1]);
        errno = EIO;
        return -1;
    }

    if (write(pipefd[1], buffer, (size_t)length) != length) {
        close(pipefd[0]);
        close(pipefd[1]);
        errno = EIO;
        return -1;
    }

    close(pipefd[1]);

    return pipefd[0];
}

/* Test-only implementation of SMS_SET_INTERVAL */
int __wrap_ioctl(int fd, unsigned long request, ...)
{
    (void)fd;

    if (request == SMS_SET_INTERVAL) {
        return 0;
    }

    errno = ENOTTY;
    return -1;
}
