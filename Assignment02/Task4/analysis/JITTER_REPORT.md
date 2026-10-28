# ASSIGNMENT 02 - TASK 4
## Real-Time Scheduling and Jitter Analysis

### 1. Test Environment

- Application: Sensor Monitoring System
- Host: Ubuntu Linux x86-64
- Sensor: Mock sensor
- Scheduling policies: SCHED_OTHER and SCHED_FIFO
- SCHED_FIFO priority: 50
- Timing clock: CLOCK_MONOTONIC
- Sleep mode: TIMER_ABSTIME
- Memory locking: mlockall(MCL_CURRENT | MCL_FUTURE)
- Both scheduling policies were tested with the same privileges.

### 2. Jitter Measurement Method

Wake-up jitter was measured in the Sensor Thread.

The actual wake-up time was compared against the
expected wake-up time:

Jitter = Actual wake-up time - Expected wake-up time

Statistics:
- Minimum
- Average
- Maximum
- P99 (99th percentile)

Jitter was recorded in microseconds.

### 3. Results at 50 ms

Duration: 60 seconds
Jitter samples per policy: 1201

| Metric | SCHED_OTHER | SCHED_FIFO |
|---|---:|---:|
| Min (us) | 58 | 14 |
| Avg (us) | 201.60 | 177.76 |
| Max (us) | 506 | 295 |
| P99 (us) | 313 | 285 |
| Produced | 668 | 668 |
| Overflow | 533 | 533 |

Histogram:

| Jitter range | OTHER | FIFO |
|---|---:|---:|
| 0-100 us | 204 | 170 |
| 100-500 us | 996 | 1031 |
| 500-1000 us | 1 | 0 |
| >=1000 us | 0 | 0 |

P99 improvement:

(313 - 285) / 313 * 100 = 8.95%

SCHED_FIFO improved P99 jitter by approximately 8.95%
in this measurement.

Both policies experienced buffer overflow at 50 ms.

### 4. Results at 500 ms

Duration: 60 seconds
Jitter samples per policy: 121

| Metric | SCHED_OTHER | SCHED_FIFO |
|---|---:|---:|
| Min (us) | 69 | 16 |
| Avg (us) | 232.53 | 110.34 |
| Max (us) | 356 | 294 |
| P99 (us) | 355 | 252 |
| Produced | 121 | 121 |
| Consumed | 121 | 121 |
| Overflow | 0 | 0 |

Histogram:

| Jitter range | OTHER | FIFO |
|---|---:|---:|
| 0-100 us | 7 | 67 |
| 100-500 us | 114 | 54 |
| 500-1000 us | 0 | 0 |
| >=1000 us | 0 | 0 |

P99 improvement:

(355 - 252) / 355 * 100 = 29.01%

Average jitter improvement:

(232.53 - 110.34) / 232.53 * 100 = 52.55%

### 5. Industrial Sensor Timing Requirement

For a 500 ms sampling interval, this report selects
a hypothetical maximum wake-up jitter of 5 ms,
equivalent to 1% of the sampling period.

This is an engineering assumption for this example,
not a universal industrial standard.

Measured maximum jitter:
- SCHED_OTHER: 356 us
- SCHED_FIFO: 294 us

Both results satisfy the selected 5000 us threshold
in these measurements.

Actual industrial requirements depend on the sensor,
control system and applicable specifications.

### 6. Buffer Overflow Analysis

At a 50 ms sampling interval:

- Total sampling cycles: 1201
- Successfully buffered samples: 668
- Buffer overflow events: 533

Overflow rate:

533 / 1201 * 100 = 44.38%

The Processor Thread has no explicit sleep operation,
but it performs message queue communication.

Blocking or delayed queue operations are possible
contributors to limited processing throughput.

The exact bottleneck has not been established.

At 500 ms, both policies processed all 121 samples
without buffer overflow.

### 7. Further Optimization

Potential improvements:

1. Investigate message queue backpressure.
2. Measure Processor Thread execution time.
3. Reduce unnecessary log formatting.
4. Consider PREEMPT_RT for stronger latency guarantees.
5. Evaluate CPU isolation and interrupt affinity.
6. Repeat measurements under controlled system load.

These are proposed improvements and have not yet
been experimentally validated.

### 8. Limitations

Measurements were performed on Ubuntu x86-64
with a mock sensor, not the actual ARM kernel driver.

The 500 ms experiments contain only 121 samples
per scheduling policy.

Results can vary with CPU load, scheduling,
hardware and operating system configuration.

Lower jitter does not guarantee zero data loss
or the absence of race conditions.

### 9. Conclusion

Both scheduling policies successfully collected
more than 1000 jitter measurements at 50 ms.

SCHED_FIFO achieved lower P99 jitter in the
observed 50 ms and 500 ms experiments.

At 500 ms, no buffer overflow was observed.

At 50 ms, both policies experienced overflow,
indicating a throughput limitation requiring
further investigation.

This report documents an AI-assisted prototype.
All measurements and conclusions should be
reviewed according to course requirements.
