#ifndef JITTER_STATS_H
#define JITTER_STATS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_JITTER_SAMPLES 4096

typedef struct {
    long values[MAX_JITTER_SAMPLES];
    size_t count;
} jitter_meter_t;

static void jitter_record(
    jitter_meter_t *meter,
    const struct timespec *expected)
{
    struct timespec actual;

    if (meter->count >= MAX_JITTER_SAMPLES)
        return;

    if (clock_gettime(CLOCK_MONOTONIC, &actual) != 0)
        return;

    long long ns =
        ((long long)actual.tv_sec - expected->tv_sec)
        * 1000000000LL
        + actual.tv_nsec - expected->tv_nsec;

    long us = (long)(ns / 1000LL);

    if (us < 0)
        us = 0;

    meter->values[meter->count++] = us;
}

static int jitter_compare(const void *a, const void *b)
{
    long x = *(const long *)a;
    long y = *(const long *)b;

    return (x > y) - (x < y);
}

static void jitter_report(
    jitter_meter_t *meter,
    const char *policy,
    unsigned int interval_ms)
{
    if (meter->count == 0) {
        printf("[JITTER] No samples collected\n");
        return;
    }

    qsort(
        meter->values,
        meter->count,
        sizeof(meter->values[0]),
        jitter_compare
    );

    long long sum = 0;
    size_t hist[4] = {0};

    for (size_t i = 0; i < meter->count; i++) {
        long us = meter->values[i];

        sum += us;

        if (us < 100)
            hist[0]++;
        else if (us < 500)
            hist[1]++;
        else if (us < 1000)
            hist[2]++;
        else
            hist[3]++;
    }

    size_t p99_index =
        (99 * meter->count + 99) / 100 - 1;

    printf(
        "[JITTER] policy=%s samples=%zu interval_ms=%u\n",
        policy, meter->count, interval_ms
    );

    printf(
        "[JITTER] min_us=%ld avg_us=%.2f max_us=%ld p99_us=%ld\n",
        meter->values[0],
        (double)sum / meter->count,
        meter->values[meter->count - 1],
        meter->values[p99_index]
    );

    printf(
        "[JITTER] histogram 0-100us=%zu "
        "100-500us=%zu 500-1000us=%zu >=1000us=%zu\n",
        hist[0], hist[1], hist[2], hist[3]
    );
}

#endif
