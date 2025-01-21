CC ?= $(CROSS_COMPILE)gcc
LD ?= $(CROSS_COMPILE)ld
CFLAGS += -Wall -Werror -pthread
LDFLAGS += -lrt

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c $(LDFLAGS)

clean:
	rm -f aesdsocket
