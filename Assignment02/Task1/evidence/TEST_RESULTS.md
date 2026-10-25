# Task 1 - Test Results

## Test 1: Sensor Monitoring - 60 Seconds

Platform: QEMU ARMv7
Kernel: Linux 5.15.0
Sampling interval: 500 ms
Duration: 60 seconds

Results:

produced=119
processed=119
consumed=119
overflow=0

Expected samples: approximately 120

SCHED_FIFO priority 50: OK

Result: PASS

## Test 2: Graceful Shutdown on QEMU

Signal: SIGINT

Results:

produced=10
processed=10
consumed=10
overflow=0
buffer_size=0

Sensor Thread stopped.
Processor Thread stopped.
Alert Thread stopped.
Logger Thread stopped.

Shutdown complete.

Result: PASS

## Test 3: Driver Statistics

read_count: 10
last_temperature: 30.44
last_humidity: 41.79

The last recorded sample matched the application log.

Result: PASS

## Notes

These results were observed during development testing.
The complete submission source and additional evidence
must be reviewed before final submission.

## Integrated Application Testing on QEMU ARMv7

### Test 1: Normal Execution (15 seconds)

Environment:
- Linux Kernel: 5.15.0
- Architecture: ARMv7
- Emulator: QEMU vexpress-a9
- Application: sms_app_personal
- Sampling interval: approximately 500 ms

Results:

Produced: 30
Consumed: 30
Processed: 30
Overflow: 0
Buffer remaining: 0

Kernel Driver Statistics:
- read_count: 30
- last_temperature: 38.09
- last_humidity: 42.21

Result: PASS

### Test 2: SIGINT Graceful Shutdown

The application was executed in continuous mode
and terminated using Ctrl+C.

Results:

Produced: 21
Consumed: 21
Processed: 21
Overflow: 0
Buffer remaining: 0

Result: PASS

### Test 3: Logging Verification

Log file: /tmp/sms.log

The final logged sensor values matched the
last temperature and humidity reported by
the kernel driver.

Result: PASS

### Test 4: Alert Processing

The application successfully displayed:
- TEMP_HIGH
- HUMID_HIGH
- TEMP_SPIKE

Result: PASS

### Test 5: Kernel IOCTL Integration

The application was updated to call SMS_SET_INTERVAL.
The integrated application completed execution
without reporting an IOCTL error.

A direct IOCTL success message should also be
confirmed in the terminal output.

Result: Functionally tested; explicit confirmation pending.
