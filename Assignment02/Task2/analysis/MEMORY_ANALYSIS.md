# ASSIGNMENT 02 - TASK 2
## Memory Management and Driver Integration

### 1. Test Environment

- Operating System: Linux Kernel 5.15
- Target architecture: ARMv7
- Emulator: QEMU vexpress-a9
- Sampling interval: 500 ms
- Test duration: 60 seconds
- Memory leak tool: Valgrind 3.22.0 on Ubuntu x86-64

### 2. Memory Footprint

| Time | VmRSS | VmSize | VmData | Threads |
|------|-------|--------|--------|---------|
| 5s   | 160 KB | 35496 KB | 33096 KB | 5 |
| 30s  | 160 KB | 35496 KB | 33096 KB | 5 |
| 55s  | 160 KB | 35496 KB | 33096 KB | 5 |

Analysis:

VmRSS remained at 160 KB throughout the observation.
VmSize remained at 35496 KB.

No continuous memory growth was observed during this
60-second test.

Stable RSS alone does not prove the absence of memory leaks.

### 3. Valgrind Analysis

The x86-64 application was tested using a mock sensor
instead of the ARM kernel driver.

Results:

- Allocations: 8
- Frees: 8
- Total allocated: 11288 bytes
- Memory in use at exit: 0 bytes
- Error summary: 0 errors

Valgrind reported:

All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts

Conclusion:

No memory leaks or memory errors were detected in the
tested application execution.

### 4. Memory Maps Analysis

The following regions were observed in /proc/PID/maps:

1. Text segment
   00483000-00486000 r-xp
   Contains executable application instructions.

2. Read-only segment
   00486000-00487000 r--p
   Contains read-only program data.

3. Writable data segment
   00487000-00488000 rw-p
   Contains writable application data.

4. Heap
   00488000-004aa000 rw-p [heap]
   Used for dynamic memory allocation.

5. Thread stack region
   74dcd000-755cd000 rw-p
   An anonymous 8 MiB mapping consistent with
   a worker thread stack.

6. Stack guard region
   74dcc000-74dcd000 ---p
   Protects against stack overflow.

7. Shared C library
   76dd0000-76ee2000 r-xp /lib/libc.so.6
   Contains C library instructions.

8. Dynamic linker
   76f0e000-76f2a000 r-xp /lib/ld-linux-armhf.so.3
   Loads and links shared libraries.

Individual worker stack identities were not verified
against thread IDs.

### 5. Kernel Driver Integration

The application uses:

- open() to access /dev/sms_sensor
- read() to receive sensor CSV data
- ioctl(SMS_SET_INTERVAL) to configure sampling
- close() to release file descriptors

Error handling:

- EAGAIN: Retry after a short delay
- EINTR: Retry interrupted operations
- EOF: Report an I/O error and retry in the next cycle

Error paths have been reviewed in source.
Individual error-injection tests were not performed.

### 6. Driver Statistics

Test A:

- Duration: 60 seconds
- Interval: 500 ms
- read_count before: 119
- read_count after: 238
- Difference: 119

Test B, after CLI updates:

- Duration: 60 seconds
- Interval: 500 ms
- Produced: 118
- Consumed: 118
- Processed: 118
- Overflow: 0
- Buffer remaining: 0
- Kernel read_count after test: 118

Required range: 110 to 130 samples.

Result: PASS

### 7. Conclusion

The tested prototype demonstrated stable memory usage,
successful driver communication and graceful shutdown.

Valgrind reported zero memory leaks and zero memory errors
for the x86-64 mock-sensor test.

The ARM driver was tested separately on QEMU.

This is an AI-assisted learning prototype. Academic
submission must follow the course's source-authorship rules.
