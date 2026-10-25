#define _POSIX_C_SOURCE 200809L

#include "sms_common.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

/* Number of samples used for rolling average */
#define WINDOW_SIZE 10

/* Log message buffer */
#define LOG_SIZE 256

/* Send a message with a timeout */
static int send_message(mqd_t queue, const void *data, size_t size)
{
    struct timespec deadline;

    if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
        return -1;

    /* Timeout: 100 milliseconds */
    deadline.tv_nsec += 100000000L;

    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    return mq_timedsend(
        queue,
        (const char *)data,
        size,
        0,
        &deadline
    );
}

/* Send processed data to Logger Thread */
static void send_log_message(sms_context_t *ctx, const char *text)
{
    char message[LOG_SIZE] = {0};

    snprintf(message, sizeof(message), "%s", text);

    if (send_message(ctx->log_q,
                     message,
                     strlen(message) + 1) != 0) {
        fprintf(stderr,
                "[Processor] Log queue error: %s\n",
                strerror(errno));
    }
}

/* Send alerts to Alert Thread */
static void send_alert_message(
    sms_context_t *ctx,
    unsigned long long timestamp,
    alert_type_t type,
    double value,
    double threshold)
{
    alert_message_t alert = {
        .timestamp_ms = timestamp,
        .type = type,
        .value = value,
        .threshold = threshold
    };

    if (send_message(ctx->alarm_q,
                     &alert,
                     sizeof(alert)) != 0) {
        fprintf(stderr,
                "[Processor] Alert queue error: %s\n",
                strerror(errno));
    }
}

/* Processor Thread */
void *processor_thread(void *arg)
{
    sms_context_t *ctx = (sms_context_t *)arg;

    /* Rolling average history */
    double temp_history[WINDOW_SIZE] = {0};
    double humid_history[WINDOW_SIZE] = {0};

    double sum_temp = 0.0;
    double sum_humid = 0.0;

    double previous_temp = 0.0;
    bool has_previous = false;

    size_t index = 0;
    size_t count = 0;

    sensor_sample_t sample;

    printf("[Processor] Thread started\n");

    while (buffer_pop(&ctx->buffer, &sample, &ctx->stop)) {

        /* Remove oldest sample when window is full */
        if (count == WINDOW_SIZE) {
            sum_temp -= temp_history[index];
            sum_humid -= humid_history[index];
        } else {
            count++;
        }

        /* Store new sample */
        temp_history[index] = sample.temperature;
        humid_history[index] = sample.humidity;

        sum_temp += sample.temperature;
        sum_humid += sample.humidity;

        /* Move circular index */
        index = (index + 1) % WINDOW_SIZE;

        /* Calculate rolling averages */
        double avg_temp = sum_temp / (double)count;
        double avg_humid = sum_humid / (double)count;

        /* Calculate temperature change */
        double temp_change = 0.0;

        if (has_previous) {
            temp_change = fabs(
                sample.temperature - previous_temp
            );
        }

        previous_temp = sample.temperature;
        has_previous = true;

        /* Update processing statistics */
        ctx->processed++;

        /* Prepare log message */
        char log_message[LOG_SIZE];

        snprintf(
            log_message,
            sizeof(log_message),
            "SAMPLE ts=%llu T=%.2f H=%.2f avgT=%.2f avgH=%.2f deltaT=%.2f",
            sample.timestamp_ms,
            sample.temperature,
            sample.humidity,
            avg_temp,
            avg_humid,
            temp_change
        );

        printf("[Processor] %s\n", log_message);

        send_log_message(ctx, log_message);

        /* Temperature threshold */
        if (sample.temperature > ctx->temp_threshold) {

            send_alert_message(
                ctx,
                sample.timestamp_ms,
                TEMP_HIGH,
                sample.temperature,
                ctx->temp_threshold
            );
        }

        /* Humidity threshold */
        if (sample.humidity > ctx->humid_threshold) {

            send_alert_message(
                ctx,
                sample.timestamp_ms,
                HUMID_HIGH,
                sample.humidity,
                ctx->humid_threshold
            );
        }

        /* Sudden temperature change */
        if (temp_change > 5.0) {

            send_alert_message(
                ctx,
                sample.timestamp_ms,
                TEMP_SPIKE,
                temp_change,
                5.0
            );
        }
    }
atomic_store(&ctx->processor_done, true);
    printf("[Processor] Thread stopped\n");

    return NULL;
}
