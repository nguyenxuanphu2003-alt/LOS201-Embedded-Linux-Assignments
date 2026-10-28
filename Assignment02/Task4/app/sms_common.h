#ifndef SMS_COMMON_H
#define SMS_COMMON_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <mqueue.h>
/* Sensor data */
typedef struct {
    unsigned long long timestamp_ms;
    double temperature;
    double humidity;
} sensor_sample_t;

/* Circular Buffer */
typedef struct {
    sensor_sample_t *items;

    size_t capacity;
    size_t head;
    size_t tail;
    size_t size;

    unsigned long long produced;
    unsigned long long consumed;
    unsigned long long overflows;

    pthread_mutex_t lock;
    pthread_cond_t has_data;

} circular_buffer_t;

/* Circular Buffer functions */
int buffer_init(circular_buffer_t *q, size_t capacity);

void buffer_destroy(circular_buffer_t *q);

bool buffer_push(circular_buffer_t *q,
                 sensor_sample_t sample);

bool buffer_pop(circular_buffer_t *q,
                sensor_sample_t *sample,
                const atomic_bool *stop);

void buffer_wake(circular_buffer_t *q);

void buffer_get_stats(
    circular_buffer_t *q,
    size_t *current_size,
    unsigned long long *total_produced,
    unsigned long long *total_consumed,
    unsigned long long *overflow_count
);
/* Application context */
typedef struct {
    const char *device_path;
    unsigned int interval_ms;
    int rt_policy;
    bool verbose;

    double temp_threshold;
    double humid_threshold;

    mqd_t alarm_q;
    mqd_t log_q;

    circular_buffer_t buffer;
    atomic_bool stop;
    atomic_bool processor_done;
    atomic_bool alert_done;
    unsigned long long processed;
} sms_context_t;
/* Sensor thread */
void *sensor_thread(void *arg);

/* Alert categories */
typedef enum {
    TEMP_HIGH,
    HUMID_HIGH,
    TEMP_SPIKE
} alert_type_t;

/* Alert information */
typedef struct {
    unsigned long long timestamp_ms;
    alert_type_t type;
    double value;
    double threshold;
} alert_message_t;

/* Processor Thread */
void *processor_thread(void *arg);
/* Alert Thread */
void *alert_thread(void *arg);

/* Logger Thread */
void *logger_thread(void *arg);
#endif
