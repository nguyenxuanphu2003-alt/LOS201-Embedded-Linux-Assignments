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
Writes processed sensor samples to /tmp/sms.log. Alert notifications are displayed by the Alert Thread.

Circular Buffer:
Uses mutex and condition variables for thread synchronization.

### Development Test Results

- Kernel driver loaded successfully on QEMU.
- Earlier practice prototype: 60 seconds, 119 samples.
- Current integrated application: 15 seconds, 30 samples.
- Current SIGINT test: 21 samples, graceful shutdown.
- SCHED_FIFO priority 50: successful.
- Circular buffer overflow: 0.
- SIGINT graceful shutdown: successful.

### Submission Status

Kernel driver and application have passed integration tests
in the development environment.

Final source review, individual implementation verification,
and submission packaging are still in progress.


### Source Provenance

This repository includes an AI-assisted learning prototype.
The application was compiled and tested on QEMU ARMv7.

AI-generated portions must not be represented as independently
authored student work. Before academic submission, the source
must be independently implemented and reviewed in accordance
with the course requirements.
