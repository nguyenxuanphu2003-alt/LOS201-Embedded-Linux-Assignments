#define _POSIX_C_SOURCE 200809L

#include "sms_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sched.h>

/* Calculate next sampling time */
static void add_ms(struct timespec *t, unsigned int ms)
{
    t->tv_sec += ms / 1000;
    t->tv_nsec += (long)(ms % 1000) * 1000000L;

    if (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

/* Read one sample from kernel driver */
static int read_sensor(const char *device, sensor_sample_t *sample)
{
    char data[128];

    int fd = open(device, O_RDONLY);

    if (fd < 0)
        return -1;

    ssize_t n = read(fd, data, sizeof(data) - 1);
    int saved_errno = errno;

    close(fd);

    if (n <= 0) {
        errno = (n == 0) ? EIO : saved_errno;
        return -1;
    }

    data[n] = '\0';

    int fields = sscanf(
        data,
        "%llu,%lf,%lf",
        &sample->timestamp_ms,
        &sample->temperature,
        &sample->humidity
    );

    if (fields != 3) {
        errno = EPROTO;
        return -1;
    }

    return 0;
}

/* Sensor Thread */
void *sensor_thread(void *arg)
{
    sms_context_t *ctx = (sms_context_t *)arg;

    struct timespec next;

    struct sched_param param = {
        .sched_priority = 50
    };

    int rc = pthread_setschedparam(
        pthread_self(),
        SCHED_FIFO,
        &param
    );

    fprintf(stderr,
            "[Sensor] SCHED_FIFO priority 50: %s\n",
            rc == 0 ? "OK" : strerror(rc));

    clock_gettime(CLOCK_MONOTONIC, &next);

    printf("[Sensor] Thread started\n");

    while (!atomic_load(&ctx->stop)) {

        sensor_sample_t sample;
        int result = -1;

        /* Retry when driver is not ready */
        for (int retry = 0; retry < 50; retry++) {

            if (atomic_load(&ctx->stop))
                break;

            result = read_sensor(
                ctx->device_path,
                &sample
            );

            if (result == 0)
                break;

            if (errno != EAGAIN && errno != EINTR) {
                perror("[Sensor] read");
                break;
            }

            struct timespec delay = {
                .tv_sec = 0,
                .tv_nsec = 10000000L
            };

            nanosleep(&delay, NULL);
        }

        if (result == 0) {

            if (buffer_push(&ctx->buffer, sample)) {

                printf(
                    "[Sensor] Timestamp=%llu Temp=%.2f Humid=%.2f\n",
                    sample.timestamp_ms,
                    sample.temperature,
                    sample.humidity
                );

            } else {
                fprintf(stderr,
                        "[Sensor] WARNING: Buffer overflow\n");
            }
        }

        add_ms(&next, ctx->interval_ms);

        int sleep_rc;

        do {
            sleep_rc = clock_nanosleep(
                CLOCK_MONOTONIC,
                TIMER_ABSTIME,
                &next,
                NULL
            );
        } while (sleep_rc == EINTR &&
                 !atomic_load(&ctx->stop));
    }

    printf("[Sensor] Thread stopped\n");

    return NULL;
}
