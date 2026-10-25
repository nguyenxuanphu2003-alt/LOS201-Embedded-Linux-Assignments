#define _POSIX_C_SOURCE 200809L

#include "sms_common.h"

#include <stdio.h>
#include <errno.h>
#include <time.h>

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

/* Convert alert type to readable text */
static const char *alert_name(alert_type_t type)
{
    switch (type) {
    case TEMP_HIGH:
        return "TEMP_HIGH";

    case HUMID_HIGH:
        return "HUMID_HIGH";

    case TEMP_SPIKE:
        return "TEMP_SPIKE";

    default:
        return "UNKNOWN";
    }
}

/* Alert Thread */
void *alert_thread(void *arg)
{
    sms_context_t *ctx = (sms_context_t *)arg;

    printf("[Alert] Thread started\n");

    while (1) {

        alert_message_t alert;
        struct timespec deadline;

        make_deadline(&deadline);

        ssize_t n = mq_timedreceive(
            ctx->alarm_q,
            (char *)&alert,
            sizeof(alert),
            NULL,
            &deadline
        );

        if (n == (ssize_t)sizeof(alert)) {

            printf(
                "[Alert] TYPE=%s TS=%llu VALUE=%.2f THRESHOLD=%.2f\n",
                alert_name(alert.type),
                alert.timestamp_ms,
                alert.value,
                alert.threshold
            );

        } else if (n >= 0) {

            fprintf(stderr,
                    "[Alert] Invalid message size\n");

        } else if (errno != ETIMEDOUT &&
                   errno != EINTR) {

            perror("[Alert] mq_timedreceive");
            break;
        }

        /* Drain alerts before shutdown */
        if (atomic_load(&ctx->processor_done)) {

            struct mq_attr attr;

            if (mq_getattr(ctx->alarm_q, &attr) == 0 &&
                attr.mq_curmsgs == 0) {
                break;
            }
        }
    }

    atomic_store(&ctx->alert_done, true);

    printf("[Alert] Thread stopped\n");

    return NULL;
}
