#define _POSIX_C_SOURCE 200809L

#include "sms_common.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>

#define LOG_MESSAGE_SIZE 256
#define LOG_FILE "/tmp/sms.log"

/* Create a timeout 100 ms from now */
static void make_deadline(struct timespec *deadline)
{
    clock_gettime(CLOCK_REALTIME, deadline);

    deadline->tv_nsec += 100000000L;

    if (deadline->tv_nsec >= 1000000000L) {
        deadline->tv_sec++;
        deadline->tv_nsec -= 1000000000L;
    }
}

/* Logger Thread */
void *logger_thread(void *arg)
{
    sms_context_t *ctx = (sms_context_t *)arg;

    FILE *fp = fopen(LOG_FILE, "a");

    if (fp == NULL) {
        perror("[Logger] fopen");
        return NULL;
    }

    printf("[Logger] Thread started\n");
    printf("[Logger] Writing to %s\n", LOG_FILE);

    while (1) {

        char message[LOG_MESSAGE_SIZE];
        struct timespec deadline;

        make_deadline(&deadline);

        ssize_t n = mq_timedreceive(
            ctx->log_q,
            message,
            sizeof(message),
            NULL,
            &deadline
        );

        if (n >= 0) {

            /* Ensure the message is null-terminated */
            if ((size_t)n >= sizeof(message)) {
                message[sizeof(message) - 1] = '\0';
            } else {
                message[n] = '\0';
            }

            fprintf(fp, "%s\n", message);
            fflush(fp);

        } else if (errno != ETIMEDOUT &&
                   errno != EINTR) {

            perror("[Logger] mq_timedreceive");
            break;
        }

        /* Wait until all producers finish */
        if (atomic_load(&ctx->processor_done) &&
            atomic_load(&ctx->alert_done)) {

            struct mq_attr attr;

            if (mq_getattr(ctx->log_q, &attr) == 0 &&
                attr.mq_curmsgs == 0) {
                break;
            }
        }
    }

    fclose(fp);

    printf("[Logger] Thread stopped\n");

    return NULL;
}
