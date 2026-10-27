# Assignment 02 - Task 2

## Memory Management and Driver Integration

This directory contains testing evidence and technical
analysis for the Sensor Monitoring System prototype.

### Test Results

- QEMU memory measurements: PASS
- Valgrind memory leak check: PASS
- Valgrind error summary: 0
- Driver read_count test: PASS
- Command-line options: Tested
- Driver error handling: Source reviewed

### Directory Structure

- analysis/MEMORY_ANALYSIS.md
- logs/memory_qemu_60s.txt
- logs/valgrind_sms.txt
- logs/valgrind_app_output.txt
- tests/mock_sensor.c

The Valgrind test uses an x86-64 mock sensor.
The actual ARM driver was tested separately on QEMU.

This is an AI-assisted learning prototype, not a
declaration of independently authored coursework.
