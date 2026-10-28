# Assignment 02 - Task 4

## Real-Time Scheduling and Jitter Analysis

### Features

- SCHED_FIFO, priority 50
- SCHED_OTHER
- CLOCK_MONOTONIC
- TIMER_ABSTIME
- mlockall()
- Wake-up jitter measurement
- Min/Avg/Max/P99 statistics
- Text histogram

### Evidence

- logs/smoke_other.txt
- logs/smoke_fifo.txt
- logs/jitter_other_50ms.txt
- logs/jitter_fifo_50ms.txt
- logs/jitter_other_500ms.txt
- logs/jitter_fifo_500ms.txt

### Technical Report

See analysis/JITTER_REPORT.md.

### Environment

The host measurements use an x86-64 mock sensor.

The ARM application was compiled, but the jitter
comparison reported here was not performed on ARM QEMU.

This is an AI-assisted learning prototype.
