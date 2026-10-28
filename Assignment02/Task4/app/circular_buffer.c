#include "sms_common.h"
#include <stdlib.h>

/* Initialize Circular Buffer */
int buffer_init(circular_buffer_t *q, size_t capacity)
{
    if (q == NULL || capacity == 0)
        return -1;

    q->items = calloc(capacity, sizeof(sensor_sample_t));

    if (q->items == NULL)
        return -1;

    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->size = 0;

    q->produced = 0;
    q->consumed = 0;
    q->overflows = 0;

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        free(q->items);
        return -1;
    }

    if (pthread_cond_init(&q->has_data, NULL) != 0) {
        pthread_mutex_destroy(&q->lock);
        free(q->items);
        return -1;
    }

    return 0;
}

/* Add sensor sample */
bool buffer_push(circular_buffer_t *q,
                 sensor_sample_t sample)
{
    bool success = false;

    pthread_mutex_lock(&q->lock);

    if (q->size < q->capacity) {

        q->items[q->tail] = sample;

        q->tail = (q->tail + 1) % q->capacity;

        q->size++;
        q->produced++;

        pthread_cond_signal(&q->has_data);

        success = true;

    } else {
        q->overflows++;
    }

    pthread_mutex_unlock(&q->lock);

    return success;
}

/* Read sensor sample */
bool buffer_pop(circular_buffer_t *q,
                sensor_sample_t *sample,
                const atomic_bool *stop)
{
    pthread_mutex_lock(&q->lock);

    while (q->size == 0 && !atomic_load(stop)) {
        pthread_cond_wait(&q->has_data, &q->lock);
    }

    if (q->size == 0) {
        pthread_mutex_unlock(&q->lock);
        return false;
    }

    *sample = q->items[q->head];

    q->head = (q->head + 1) % q->capacity;

    q->size--;
    q->consumed++;

    pthread_mutex_unlock(&q->lock);

    return true;
}

/* Wake up waiting threads */
void buffer_wake(circular_buffer_t *q)
{
    pthread_mutex_lock(&q->lock);

    pthread_cond_broadcast(&q->has_data);

    pthread_mutex_unlock(&q->lock);
}

/* Read buffer statistics */
void buffer_get_stats(
    circular_buffer_t *q,
    size_t *current_size,
    unsigned long long *total_produced,
    unsigned long long *total_consumed,
    unsigned long long *overflow_count)
{
    pthread_mutex_lock(&q->lock);

    if (current_size != NULL)
        *current_size = q->size;

    if (total_produced != NULL)
        *total_produced = q->produced;

    if (total_consumed != NULL)
        *total_consumed = q->consumed;

    if (overflow_count != NULL)
        *overflow_count = q->overflows;

    pthread_mutex_unlock(&q->lock);
}

/* Release Circular Buffer resources */
void buffer_destroy(circular_buffer_t *q)
{
    pthread_cond_destroy(&q->has_data);
    pthread_mutex_destroy(&q->lock);

    free(q->items);
    q->items = NULL;
}
