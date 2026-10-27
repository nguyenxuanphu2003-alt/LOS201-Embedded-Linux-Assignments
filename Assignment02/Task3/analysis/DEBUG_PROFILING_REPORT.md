# ASSIGNMENT 02 - TASK 3
## Debugging and Performance Profiling

### 1. Test Environment

- Operating system: Ubuntu Linux
- Host architecture: x86-64
- GDB version: 15.1
- Application: sms_app_x86_mock
- Sensor: Mock sensor for host-side testing
- Tools: GDB, strace, perf

Note:
The profiling application uses a mock sensor rather
than the real ARM kernel driver.

### 2. GDB Debugging

The application uses five threads:

1. Main Thread
2. Sensor Thread
3. Processor Thread
4. Alert Thread
5. Logger Thread

Breakpoints were placed at:

- buffer_push()
- buffer_pop()

At entry to buffer_push():

head = 0
tail = 0
size = 0

At entry to buffer_pop():

head = 1
tail = 1
size = 0

These values represent snapshots at function entry.
They demonstrate observation of the producer and
consumer paths, but do not independently prove
the presence or absence of race conditions.

The following commands were used:

- info threads
- thread apply all bt
- print q->head
- print q->tail
- print q->size

All five threads were successfully inspected.

### 3. strace Analysis

Test duration: 30 seconds
Sampling interval: 500 ms

Total system calls: 1707
System calls reporting errors: 532

Top five system calls by number of calls:

| System call | Calls | Purpose |
|---|---:|---|
| mq_timedreceive | 608 | Receive queue messages |
| clock_nanosleep | 360 | Thread timing |
| futex | 191 | Thread synchronization |
| close | 129 | Close file descriptors |
| write | 125 | Write application output |

The 527 errors reported for mq_timedreceive
may include normal timeout conditions.

The specific errno values were not established
by the summary-only strace run.

Because the application uses a mock sensor,
some pipe and file operations are artifacts
of the test environment.

### 4. CPU Time Analysis

Measured values:

real = 30.50 seconds
user = 0.02 seconds
sys  = 0.02 seconds

Total CPU time = 0.04 seconds.

The program spends most of its elapsed time
waiting for sampling intervals, message queues
and thread synchronization.

The application is primarily wait-oriented
rather than CPU-bound in this test.

User and system CPU times are both small.
Their measured ratio is approximately 1:1.

### 5. perf CPU Profiling

Command used:

sudo perf record -g

Sampling interval: 50 ms
Test duration: 30 seconds

perf record reported 1056 collected samples
across its recorded events.

The displayed cpu_atom/cycles/P event contained
851 samples and reported zero lost samples.

Selected CPU hotspots:

| Function | Overhead |
|---|---:|
| __vfscanf_internal | 4.69% |
| __memcg_slab_post_alloc_hook | 2.73% |
| __printf_buffer | 2.71% |
| __printf_fp_buffer_1.isra.0 | 2.49% |
| __GI_____strtod_l_internal | 2.14% |

The overhead percentages above correspond to
the displayed cpu_atom/cycles/P event.

The most prominent displayed hotspot was
__vfscanf_internal.

This function is used by sscanf() during
sensor CSV parsing.

### 6. Proposed Optimization

Current implementation:

read_sensor() uses sscanf() to parse CSV.

Proposed improvement:

Evaluate replacing sscanf() with dedicated
numeric parsing functions, such as strtoull()
and strtod(), with strict input validation.

Expected benefit:

Potentially reduce CSV parsing overhead.

This is a proposed optimization only.
No comparative benchmark has been performed.

A second possible improvement is to avoid
unnecessary string formatting for every sample.

### 7. Stress Test

Test configuration:

- Sampling interval: 50 ms
- Duration: 30 seconds

Normal execution:

Produced: 601
Consumed: 601
Processed: 601
Overflow: 0

Execution under sudo perf:

Produced: 369
Consumed: 369
Processed: 369
Overflow: 232

During the sudo perf run, SCHED_FIFO priority 50
was successfully enabled for the Sensor Thread.

During normal execution, the attempt to enable
SCHED_FIFO was rejected.

The scheduling difference may contribute to
the overflow observed under sudo perf.

However, profiling and privilege differences
were not fully isolated, so causation has not
been established.

This behavior is a candidate for further
investigation in Task 4.

### 8. Conclusion

GDB successfully inspected all five threads
and the circular buffer producer/consumer paths.

strace identified frequent message queue,
sleep and synchronization operations.

perf identified CSV parsing as a relevant
CPU hotspot in the test environment.

The normal 50 ms stress test processed
601 samples without buffer overflow.

All conclusions are limited to the tests
and environments described above.

This is an AI-assisted technical analysis
of the student's collected test evidence.
