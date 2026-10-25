# Assignment 02 - Task 1
## Sensor Monitoring System

### Development Environment
- Ubuntu Linux
- Linux Kernel 5.15.0
- ARM Cortex-A9 (ARMv7)
- QEMU vexpress-a9
- GCC ARM cross-compiler
- POSIX Threads

### Project Structure

driver/
- sms_sensor_driver.c
- sms_sensor.h
- asgn1_ioctl.h
- Makefile

app/
- Sensor Thread
- Processor Thread
- Alert Thread
- Logger Thread
- Circular Buffer
- POSIX Message Queues

evidence/
- Test results and verification records

### Kernel Driver

Device: /dev/sms_sensor

Procfs: /proc/sms_stats

IOCTL: SMS_SET_INTERVAL

The driver generates simulated temperature and humidity data.

### Build Kernel Driver

cd driver
make

The driver is built against Linux Kernel 5.15.0
for the ARMv7 architecture.

### Application Architecture

Sensor Thread:
Reads sensor data from the kernel driver.

Processor Thread:
Calculates rolling averages and detects abnormal readings.

Alert Thread:
Receives and displays alerts through POSIX Message Queues.

Logger Thread:
Writes monitoring data and alerts to a log file.

Circular Buffer:
Uses mutex and condition variables for thread synchronization.

### Development Test Results

- Kernel driver loaded successfully on QEMU.
- 60-second test at 500 ms: 119 samples.
- SCHED_FIFO priority 50: successful.
- Circular buffer overflow: 0.
- SIGINT graceful shutdown: successful.

### Submission Status

Kernel driver and application have passed integration tests
in the development environment.

Final source review, individual implementation verification,
and submission packaging are still in progress.
