OBJS=update_smb
CFLAGS+= -Wall
GCC=$(CROSS_COMPILE)gcc

all:	$(OBJS)

update_smb: update_smb.c
	$(GCC) $(CFLAGS) -o $@ $< $(EXTRA_LDFLAGS) $(LDFLAGS) -lconfig

clean:
	rm -f *.o
	rm -f $(OBJS)
