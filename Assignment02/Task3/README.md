# Assignment 02 - Task 3

## Debugging and Performance Profiling

This task investigates the Sensor Monitoring System
using GDB, strace and perf.

### 1. GDB

Debugging of five application threads.

Evidence:

- logs/gdb_sms.txt
- logs/gdb_sms_push_pop.txt

### 2. strace

System call analysis over 30 seconds.

Evidence:

- logs/strace_sms.txt
- logs/strace_app_output.txt
- logs/time_sms.txt

### 3. perf

CPU profiling of the x86-64 mock-sensor application.

Evidence:

- logs/perf_sms.txt
- logs/perf_app_output.txt

### 4. Stress Testing

Normal 50 ms / 30 second test:

- Produced: 601
- Processed: 601
- Overflow: 0

Evidence:

- logs/stress_50ms_30s.txt

### 5. Technical Analysis

See:

analysis/DEBUG_PROFILING_REPORT.md

### Limitations

The host tests use a mock sensor.

The perf test was performed with sudo,
which changed the application's scheduling
permissions.

The results are not direct measurements
of the ARM kernel driver.

The materials are part of an AI-assisted
learning prototype and must be reviewed
under the course's academic integrity rules.
