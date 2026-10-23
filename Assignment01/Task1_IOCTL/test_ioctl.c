#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "asgn1_ioctl.h"

int main(void)
{
    int fd;
    int mode = 1;
    struct asgn1_stats stats;
    char version[64] = {0};

    fd = open("/dev/asgn1", O_RDWR);

    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("=== IOCTL TEST ===\n");

    if (ioctl(fd, ASGN1_GET_VERSION, version) == 0)
        printf("Version: %s\n", version);
    else
        perror("GET_VERSION");

    if (ioctl(fd, ASGN1_GET_STATS, &stats) == 0) {
        printf("Open count: %d\n", stats.open_count);
        printf("Write count: %d\n", stats.write_count);
        printf("Read count: %d\n", stats.read_count);
        printf("Buffer length: %d\n", stats.buffer_len);
        printf("Last write size: %d\n", stats.last_write_size);
    } else {
        perror("GET_STATS");
    }

    /* TEST: READ-ONLY MODE */
 printf("\n[TEST] SET_MODE = 0 (Read-only)\n");
 mode = 0;

if (ioctl(fd, ASGN1_SET_MODE, &mode) == 0) {
    printf("SET_MODE 0: PASS\n");
} else {
    perror("SET_MODE 0");
}

/* Try writing while read-only */
errno = 0;

if (write(fd, "Blocked write", 13) == -1 &&
    errno == EPERM) {
    printf("Read-only protection: PASS\n");
} else {
    printf("Read-only protection: FAIL\n");
}

/* TEST: READ-WRITE MODE */
printf("\n[TEST] SET_MODE = 1 (Read/Write)\n");

mode = 1;

if (ioctl(fd, ASGN1_SET_MODE, &mode) == 0) {
    printf("SET_MODE 1: PASS\n");
} else {
    perror("SET_MODE 1");
}

/* Try writing while read-write */
if (write(fd, "Allowed write", 13) == 13) {
    printf("Read-write permission: PASS\n");
} else {
    perror("Read-write test");
}

    if (ioctl(fd, ASGN1_RESET_BUFFER) == 0)
        printf("RESET_BUFFER: PASS\n");
    else
        perror("RESET_BUFFER");

    if (ioctl(fd, ASGN1_GET_STATS, &stats) == 0)
        printf("Buffer length after reset: %d\n",
               stats.buffer_len);
    else
        perror("GET_STATS after reset");

/* Test invalid mode */
printf("\n[TEST] INVALID MODE\n");

mode = 2;

if (ioctl(fd, ASGN1_SET_MODE, &mode) == -1 &&
    errno == EINVAL) {
    printf("Invalid mode protection: PASS\n");
} else {
    printf("Invalid mode protection: FAIL\n");
}    
close(fd);

    return 0;
}
