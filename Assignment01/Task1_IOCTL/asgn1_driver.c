#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include "asgn1_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nguyen Xuan Phu");
MODULE_DESCRIPTION("Assignment 01 - Character Device with ioctl");
MODULE_VERSION("1.0");

#define DEVICE_NAME "asgn1"
#define CLASS_NAME "asgn1_class"
#define BUFFER_SIZE 1024
#define DRIVER_MAJOR 241

static int major_number = DRIVER_MAJOR;

static char device_buffer[BUFFER_SIZE];
static int buffer_len = 0;
static int open_count = 0;

static int write_count = 0;
static int read_count = 0;
static int last_write_size = 0;

static int driver_mode = 1;
static struct class *lab2_class = NULL;
static struct device *lab2_device = NULL;

static DEFINE_MUTEX(lab2_mutex);


/* =========================================================
 * Character Device
 * ========================================================= */

static int lab2_open(struct inode *inode, struct file *file)
{
    int count;

    mutex_lock(&lab2_mutex);

    open_count++;
    count = open_count;

    mutex_unlock(&lab2_mutex);

    pr_info("asgn1: device opened (count=%d)\n", count);

    return 0;
}

static int lab2_release(struct inode *inode, struct file *file)
{
    pr_info("lab2_driver: device closed\n");

    return 0;
}


static ssize_t lab2_read(struct file *file,
                         char __user *buf,
                         size_t len,
                         loff_t *offset)
{
    ssize_t bytes_read;

    mutex_lock(&lab2_mutex);

    if (*offset >= buffer_len) {
        mutex_unlock(&lab2_mutex);
        return 0;
    }

    if (len > buffer_len - *offset)
        len = buffer_len - *offset;

    if (copy_to_user(buf,
                     device_buffer + *offset,
                     len)) {

        mutex_unlock(&lab2_mutex);

        pr_err("lab2_driver: copy_to_user failed\n");

        return -EFAULT;
    }

    *offset += len;
    bytes_read = len;
    read_count++;
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: sent %zd bytes to user\n",
            bytes_read);

    return bytes_read;
}


static ssize_t lab2_write(struct file *file,
                          const char __user *buf,
                          size_t len,
                          loff_t *offset)
{
    if (len > BUFFER_SIZE - 1) {
        pr_warn("lab2_driver: write too large, truncating\n");

        len = BUFFER_SIZE - 1;
    }

    mutex_lock(&lab2_mutex);

    /* Check driver mode before writing */
    if (driver_mode == 0) {
        mutex_unlock(&lab2_mutex);

        pr_warn("asgn1: write denied in read-only mode\n");

        return -EPERM;
    }
    if (copy_from_user(device_buffer, buf, len)) {

        mutex_unlock(&lab2_mutex);

        pr_err("lab2_driver: copy_from_user failed\n");

        return -EFAULT;
    }

    buffer_len = len;

    device_buffer[buffer_len] = '\0';
    write_count++;
    last_write_size = len;
    mutex_unlock(&lab2_mutex);

    pr_info("lab2_driver: received %zu bytes: [%s]\n",
            len,
            device_buffer);

    return len;
}
/* =========================================================
 * IOCTL Operations
 * ========================================================= */

static long asgn1_ioctl(struct file *file,
                        unsigned int cmd,
                        unsigned long arg)
{
    struct asgn1_stats stats;
    int mode;
    char version[64] = "Assignment 01 Driver v1.0";

    switch (cmd) {

    case ASGN1_RESET_BUFFER:

        mutex_lock(&lab2_mutex);

        memset(device_buffer, 0, BUFFER_SIZE);
        buffer_len = 0;

        mutex_unlock(&lab2_mutex);

        pr_info("asgn1: buffer reset\n");

        return 0;

    case ASGN1_GET_STATS:

        mutex_lock(&lab2_mutex);

        stats.open_count = open_count;
        stats.write_count = write_count;
        stats.read_count = read_count;
        stats.buffer_len = buffer_len;
        stats.last_write_size = last_write_size;

        mutex_unlock(&lab2_mutex);

        if (copy_to_user((void __user *)arg,
                         &stats, sizeof(stats)))
            return -EFAULT;

        return 0;

    case ASGN1_SET_MODE:

        if (copy_from_user(&mode,
                           (void __user *)arg,
                           sizeof(mode)))
            return -EFAULT;

        if (mode != 0 && mode != 1)
            return -EINVAL;

        mutex_lock(&lab2_mutex);

        driver_mode = mode;

        mutex_unlock(&lab2_mutex);

        pr_info("asgn1: mode changed to %d\n", mode);

        return 0;

    case ASGN1_GET_VERSION:

        if (copy_to_user((void __user *)arg,
                         version, sizeof(version)))
            return -EFAULT;

        return 0;

    default:
        return -ENOTTY;
    }
}

static const struct file_operations lab2_fops = {
    .owner = THIS_MODULE,
    .open = lab2_open,
    .read = lab2_read,
    .write = lab2_write,
    .release = lab2_release,
    .unlocked_ioctl = asgn1_ioctl,
};


/* =========================================================
 * PROCFS
 * /proc/lab2_info
 * ========================================================= */

static int lab2_proc_show(struct seq_file *m,
                          void *v)
{
    mutex_lock(&lab2_mutex);

    seq_printf(m,
               "LAB-02 Character Device Driver\n");

    seq_printf(m,
               "==============================\n");

    seq_printf(m,
               "Device name : %s\n",
               DEVICE_NAME);

    seq_printf(m,
               "Major       : %d\n",
               major_number);

    seq_printf(m,
               "Buffer size : %d\n",
               BUFFER_SIZE);

    seq_printf(m,
               "Buffer len  : %d\n",
               buffer_len);

    seq_printf(m,
               "Open count  : %d\n",
               open_count);

    seq_printf(m,
               "Last data   : %s\n",
               buffer_len > 0 ? device_buffer : "(empty)");

    mutex_unlock(&lab2_mutex);

    return 0;
}


static int lab2_proc_open(struct inode *inode,
                          struct file *file)
{
    return single_open(file,
                       lab2_proc_show,
                       NULL);
}


static const struct proc_ops lab2_proc_ops = {
    .proc_open    = lab2_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};


/* =========================================================
 * SYSFS
 * /sys/class/lab2_class/lab2/
 * ========================================================= */

static ssize_t buffer_len_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);

    ret = scnprintf(buf,
                    PAGE_SIZE,
                    "%d\n",
                    buffer_len);

    mutex_unlock(&lab2_mutex);

    return ret;
}


static ssize_t open_count_show(struct device *dev,
                               struct device_attribute *attr,
                               char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);

    ret = scnprintf(buf,
                    PAGE_SIZE,
                    "%d\n",
                    open_count);

    mutex_unlock(&lab2_mutex);

    return ret;
}


static ssize_t last_data_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
{
    ssize_t ret;

    mutex_lock(&lab2_mutex);

    ret = scnprintf(buf,
                    PAGE_SIZE,
                    "%s\n",
                    buffer_len > 0 ?
                    device_buffer :
                    "(empty)");

    mutex_unlock(&lab2_mutex);

    return ret;
}


static DEVICE_ATTR_RO(buffer_len);
static DEVICE_ATTR_RO(open_count);
static DEVICE_ATTR_RO(last_data);


static struct attribute *lab2_attrs[] = {
    &dev_attr_buffer_len.attr,
    &dev_attr_open_count.attr,
    &dev_attr_last_data.attr,
    NULL,
};


static const struct attribute_group lab2_attr_group = {
    .attrs = lab2_attrs,
};


/* =========================================================
 * MODULE INIT
 * ========================================================= */

static int __init lab2_init(void)
{
    int ret;

    pr_info("lab2_driver: initializing module\n");


    /* Register character device */

    ret = register_chrdev(major_number,
                          DEVICE_NAME,
                          &lab2_fops);

    if (ret < 0) {
        pr_err("lab2_driver: register_chrdev failed: %d\n",
               ret);

        return ret;
    }


    /* Create class */

    lab2_class = class_create(THIS_MODULE,
                              CLASS_NAME);

    if (IS_ERR(lab2_class)) {

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return PTR_ERR(lab2_class);
    }


    /* Create device */

    lab2_device = device_create(
        lab2_class,
        NULL,
        MKDEV(major_number, 0),
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(lab2_device)) {

        class_destroy(lab2_class);

        unregister_chrdev(major_number,
                          DEVICE_NAME);

        return PTR_ERR(lab2_device);
    }


    /* Create sysfs attributes */

    ret = sysfs_create_group(
        &lab2_device->kobj,
        &lab2_attr_group
    );

    if (ret) {

        device_destroy(
            lab2_class,
            MKDEV(major_number, 0)
        );

        class_destroy(lab2_class);

        unregister_chrdev(
            major_number,
            DEVICE_NAME
        );

        return ret;
    }


    /* Create procfs */

    if (!proc_create("lab2_info",
                     0444,
                     NULL,
                     &lab2_proc_ops)) {

        sysfs_remove_group(
            &lab2_device->kobj,
            &lab2_attr_group
        );

        device_destroy(
            lab2_class,
            MKDEV(major_number, 0)
        );

        class_destroy(lab2_class);

        unregister_chrdev(
            major_number,
            DEVICE_NAME
        );

        return -ENOMEM;
    }


    pr_info(
        "lab2_driver: loaded successfully, major=%d\n",
        major_number
    );

    pr_info(
        "lab2_driver: /proc/lab2_info created\n"
    );

    pr_info(
        "lab2_driver: sysfs attributes created\n"
    );

    return 0;
}


/* =========================================================
 * MODULE EXIT
 * ========================================================= */

static void __exit lab2_exit(void)
{
    remove_proc_entry("lab2_info", NULL);

    sysfs_remove_group(
        &lab2_device->kobj,
        &lab2_attr_group
    );

    device_destroy(
        lab2_class,
        MKDEV(major_number, 0)
    );

    class_destroy(lab2_class);

    unregister_chrdev(
        major_number,
        DEVICE_NAME
    );

    pr_info(
        "lab2_driver: module unloaded\n"
    );
}


module_init(lab2_init);
module_exit(lab2_exit);
