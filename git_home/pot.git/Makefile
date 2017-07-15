#
#	Copyright (C) 2008 Delta Networks Inc.
#

CC = gcc

FILES = pot.c
VALFILES = potval.c
NTPFILES = ntpst.c
STAFILES = stamac.c
CFLAGS += -Wall -Wunused -g
LDFLAGS += -lconfig

all : potd potval ntpst stamac

potd : $(FILES)
	$(CC) $(CFLAGS) $(FILES) -o $@ $(EXTRA_LDFLAGS) $(LDFLAGS)
	@echo "make $@ finished on `date`"

potval: $(VALFILES)
	$(CC) $(CFLAGS) $(VALFILES) -o $@ $(EXTRA_LDFLAGS) $(LDFLAGS)
	@echo "make $@ finished on `date`"

ntpst: $(NTPFILES)
	$(CC) $(CFLAGS) $(NTPFILES) -o $@ $(EXTRA_LDFLAGS) $(LDFLAGS)
	@echo "make $@ finished on `date`"

stamac: $(STAFILES)
	$(CC) $(CFLAGS) $(STAFILES) -o $@ $(EXTRA_LDFLAGS) $(LDFLAGS)
	@echo "make $@ finished on `date`"

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o 
	rm -f potd potval ntpst stamac
