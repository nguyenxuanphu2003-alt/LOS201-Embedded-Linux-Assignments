CC_HOST = gcc
CC_ARM = arm-linux-gnueabihf-gcc

CFLAGS = -std=gnu11 -O2 -Wall -Wextra -Werror -pthread
LDLIBS = -pthread -lrt -lm

SOURCES = \
	sms_app.c \
	circular_buffer.c \
	sensor_thread.c \
	processor_thread.c \
	alert_thread.c \
	logger_thread.c

.PHONY: all host arm clean

all: host arm

host: sms_app_x86

arm: sms_app

sms_app_x86: $(SOURCES) sms_common.h
	$(CC_HOST) $(CFLAGS) -o $@ $(SOURCES) $(LDLIBS)

sms_app: $(SOURCES) sms_common.h
	$(CC_ARM) $(CFLAGS) -o $@ $(SOURCES) $(LDLIBS)

clean:
	rm -f sms_app sms_app_x86 *.o
