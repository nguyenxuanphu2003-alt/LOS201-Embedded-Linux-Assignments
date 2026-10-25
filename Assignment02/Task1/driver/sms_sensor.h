/*
 * ASSIGNMENT 02 - TASK 1
 * Sensor Monitoring System - IOCTL Header
 *
 * Student: Nguyen Xuan Phu
 * Student ID: 
 * University: FPT University
 *
 * Target: Linux Kernel 5.15 - ARMv7
 */

#ifndef SMS_SENSOR_H
#define SMS_SENSOR_H

#include <linux/ioctl.h>

/* ==========================================
 * IOCTL DEFINITIONS
 * ========================================== */

/* Magic number */
#define SMS_IOC_MAGIC 'S'

/* Default sampling interval: 100 ms */
#define SMS_DEFAULT_INTERVAL 100

/* Set sensor reading interval */
#define SMS_SET_INTERVAL \
    _IOW(SMS_IOC_MAGIC, 1, unsigned int)

#endif /* SMS_SENSOR_H */
