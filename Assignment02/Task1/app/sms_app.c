#define _POSIX_C_SOURCE 200809L

#include "sms_common.h"
#include "../driver/sms_sensor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#define DEVICE_PATH "/dev/sms_sensor"

#define ALERT_QUEUE "/sms_alert_task1"
#define LOG_QUEUE   "/sms_log_task1"

#define BUFFER_CAPACITY 64
#define DEFAULT_INTERVAL_MS 500
#define DEFAULT_DURATION_SEC 60

static volatile sig_atomic_t interrupted = 0;

/* SIGINT handler */
static void handle_sigint(int signo)
{
    (void)signo;
    interrupted = 1;
}

/* Create POSIX Message Queues */
static int setup_queues(sms_context_t *ctx)
{
    struct mq_attr alert_attr = {0};
    struct mq_attr log_attr = {0};

    alert_attr.mq_maxmsg = 10;
    alert_attr.mq_msgsize = sizeof(alert_message_t);

    log_attr.mq_maxmsg = 10;
    log_attr.mq_msgsize = 256;

    mq_unlink(ALERT_QUEUE);
    mq_unlink(LOG_QUEUE);

    ctx->alarm_q = mq_open(
        ALERT_QUEUE,
        O_CREAT | O_RDWR,
        0600,
        &alert_attr
    );

    if (ctx->alarm_q == (mqd_t)-1) {
        perror("[Main] mq_open alert");
        return -1;
    }

    ctx->log_q = mq_open(
        LOG_QUEUE,
        O_CREAT | O_RDWR,
        0600,
        &log_attr
    );

    if (ctx->log_q == (mqd_t)-1) {
        perror("[Main] mq_open log");
        mq_close(ctx->alarm_q);
        ctx->alarm_q = (mqd_t)-1;
        mq_unlink(ALERT_QUEUE);
        return -1;
    }

    return 0;
}

/* Release POSIX Message Queues */
static void cleanup_queues(sms_context_t *ctx)
{
    if (ctx->alarm_q != (mqd_t)-1)
        mq_close(ctx->alarm_q);

    if (ctx->log_q != (mqd_t)-1)
        mq_close(ctx->log_q);

    mq_unlink(ALERT_QUEUE);
    mq_unlink(LOG_QUEUE);
}


/* Parse an unsigned integer argument */
static bool parse_uint_arg(
    const char *text,
    unsigned int *out,
    unsigned int min,
    unsigned int max)
{
    char *end = NULL;

    if (!text || !*text || *text == '-')
        return false;

    errno = 0;
    unsigned long value = strtoul(text, &end, 10);

    if (errno != 0 || end == text ||
        *end != '\0' || value < min || value > max)
        return false;

    *out = (unsigned int)value;
    return true;
}

/* Parse a floating-point threshold */
static bool parse_double_arg(
    const char *text,
    double *out,
    double min,
    double max)
{
    char *end = NULL;

    errno = 0;
    double value = strtod(text, &end);

    if (errno != 0 || end == text ||
        *end != '\0' || !isfinite(value) ||
        value < min || value > max)
        return false;

    *out = value;
    return true;
}

static void print_usage(const char *program)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS]\n"
        "  -i <ms>   Sampling interval (default: 500)\n"
        "  -t <C>    Temperature threshold (default: 40)\n"
        "  -h <%%>    Humidity threshold (default: 80)\n"
        "  -d <sec>  Duration, 0 = infinite (default: 60)\n"
        "  -v        Verbose sensor output\n",
        program);
}

/* Main Application */
int main(int argc, char *argv[])
{
    unsigned int duration = DEFAULT_DURATION_SEC;
    unsigned int interval = DEFAULT_INTERVAL_MS;

    double temp_threshold = 40.0;
    double humid_threshold = 80.0;

    bool verbose = false;
    bool valid = true;

    /* Preserve the old positional duration syntax */
    if (argc == 2 && argv[1][0] != '-') {
        valid = parse_uint_arg(
            argv[1], &duration, 0, 3600
        );
    } else {
        int opt;

        opterr = 0;

        while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
            switch (opt) {
            case 'i':
                valid = parse_uint_arg(
                    optarg, &interval, 1, 60000
                );
                break;

            case 't':
                valid = parse_double_arg(
                    optarg, &temp_threshold, 0.0, 100.0
                );
                break;

            case 'h':
                valid = parse_double_arg(
                    optarg, &humid_threshold, 0.0, 100.0
                );
                break;

            case 'd':
                valid = parse_uint_arg(
                    optarg, &duration, 0, 3600
                );
                break;

            case 'v':
                verbose = true;
                break;

            default:
                valid = false;
                break;
            }

            if (!valid)
                break;
        }

        if (optind != argc)
            valid = false;
    }

    if (!valid) {
        fprintf(stderr, "Invalid command-line arguments\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    sms_context_t ctx = {0};

    ctx.device_path = DEVICE_PATH;
    ctx.interval_ms = interval;

    ctx.temp_threshold = temp_threshold;
    ctx.humid_threshold = humid_threshold;
    ctx.verbose = verbose;

    ctx.alarm_q = (mqd_t)-1;
    ctx.log_q = (mqd_t)-1;

    atomic_init(&ctx.stop, false);
    atomic_init(&ctx.processor_done, false);
    atomic_init(&ctx.alert_done, false);

    ctx.processed = 0;

    /* Configure sampling interval in kernel driver */
    {
        int fd = open(ctx.device_path, O_RDONLY);

        if (fd < 0) {
            perror("[Main] open sensor");
            return EXIT_FAILURE;
        }

        if (ioctl(fd, SMS_SET_INTERVAL, &ctx.interval_ms) < 0) {
            perror("[Main] SMS_SET_INTERVAL");
            close(fd);
            return EXIT_FAILURE;
        }

        close(fd);

        printf("[Main] Kernel sampling interval set to %u ms\n",
               ctx.interval_ms);
    }

    struct sigaction sa = {0};

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("[Main] sigaction");
        return EXIT_FAILURE;
    }

    if (buffer_init(&ctx.buffer, BUFFER_CAPACITY) != 0) {
        fprintf(stderr, "[Main] Buffer initialization failed\n");
        return EXIT_FAILURE;
    }

    if (setup_queues(&ctx) != 0) {
        buffer_destroy(&ctx.buffer);
        return EXIT_FAILURE;
    }

    pthread_t sensor_tid;
    pthread_t processor_tid;
    pthread_t alert_tid;
    pthread_t logger_tid;

    bool sensor_started = false;
    bool processor_started = false;
    bool alert_started = false;
    bool logger_started = false;

    int status = EXIT_SUCCESS;
    int rc;

    printf("====================================\n");
    printf(" Sensor Monitoring System - Task 1\n");
    printf("====================================\n");

    printf("[Main] Device: %s\n", ctx.device_path);
    printf("[Main] Requested interval: %u ms\n",
           ctx.interval_ms);
    printf("[Main] Temperature threshold: %.2f\n",
           ctx.temp_threshold);
    printf("[Main] Humidity threshold: %.2f\n",
           ctx.humid_threshold);

    /* Start consumers before producers */

    rc = pthread_create(
        &logger_tid, NULL, logger_thread, &ctx
    );

    if (rc != 0) {
        fprintf(stderr, "[Main] Logger: %s\n", strerror(rc));
        status = EXIT_FAILURE;
        goto shutdown;
    }

    logger_started = true;

    rc = pthread_create(
        &alert_tid, NULL, alert_thread, &ctx
    );

    if (rc != 0) {
        fprintf(stderr, "[Main] Alert: %s\n", strerror(rc));
        status = EXIT_FAILURE;
        goto shutdown;
    }

    alert_started = true;

    rc = pthread_create(
        &processor_tid, NULL, processor_thread, &ctx
    );

    if (rc != 0) {
        fprintf(stderr, "[Main] Processor: %s\n", strerror(rc));
        status = EXIT_FAILURE;
        goto shutdown;
    }

    processor_started = true;

    rc = pthread_create(
        &sensor_tid, NULL, sensor_thread, &ctx
    );

    if (rc != 0) {
        fprintf(stderr, "[Main] Sensor: %s\n", strerror(rc));
        status = EXIT_FAILURE;
        goto shutdown;
    }

    sensor_started = true;

    printf("[Main] All threads started\n");

    if (duration == 0) {
        printf("[Main] Press Ctrl+C to stop\n");
    } else {
        printf("[Main] Running for %u seconds\n", duration);
    }

    struct timespec start;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &start);

    while (!interrupted) {

        if (duration != 0) {

            clock_gettime(CLOCK_MONOTONIC, &now);

            double elapsed =
                (double)(now.tv_sec - start.tv_sec) +
                (double)(now.tv_nsec - start.tv_nsec) / 1e9;

            if (elapsed >= duration)
                break;
        }

        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = 100000000L
        };

        nanosleep(&delay, NULL);
    }

shutdown:

    printf("[Main] Shutting down...\n");

    atomic_store(&ctx.stop, true);
    buffer_wake(&ctx.buffer);

    if (sensor_started)
        pthread_join(sensor_tid, NULL);

    if (processor_started) {
        pthread_join(processor_tid, NULL);
    } else {
        atomic_store(&ctx.processor_done, true);
    }

    if (alert_started) {
        pthread_join(alert_tid, NULL);
    } else {
        atomic_store(&ctx.alert_done, true);
    }

    if (logger_started)
        pthread_join(logger_tid, NULL);

    size_t remaining = 0;

    unsigned long long produced = 0;
    unsigned long long consumed = 0;
    unsigned long long overflow = 0;

    buffer_get_stats(
        &ctx.buffer,
        &remaining,
        &produced,
        &consumed,
        &overflow
    );

    printf("\n========== FINAL STATISTICS ==========\n");
    printf("Produced: %llu\n", produced);
    printf("Consumed: %llu\n", consumed);
    printf("Processed: %llu\n", ctx.processed);
    printf("Overflow: %llu\n", overflow);
    printf("Buffer remaining: %zu\n", remaining);
    printf("======================================\n");

    cleanup_queues(&ctx);
    buffer_destroy(&ctx.buffer);

    printf("[Main] Application stopped\n");

    return status;
}
