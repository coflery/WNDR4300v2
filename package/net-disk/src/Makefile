#
#	Copyright (C) 2009 Delta Networks Inc.
#

CC = gcc

FILES = udisk.o uevent.o

CFLAGS += -Wall -Wunused -g -O2

all : net-disk usb_led

net-disk : $(FILES)
	$(CC) $(CFLAGS) $(FILES) -o $@ $(EXTRA_LDFLAGS) $(LDFLAGS)
	@echo "make $@ finished on `date`"

usb_led: usbled.o
	$(CC) $(CFLAGS) usbled.o  -o $@ $(EXTRA_LDFLAGS) $(LDFLAGS) -lconfig
	@echo "make $@ finished on `date`"

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o 
	rm -f net-disk
