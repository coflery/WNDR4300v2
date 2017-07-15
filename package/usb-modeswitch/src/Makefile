PROG        = usb_modeswitch
VERS        = 0.9.7
CC          = gcc
OBJS        = usb_modeswitch.c
PREFIX	    = $(DESTDIR)/usr
ETCDIR	    = $(DESTDIR)/etc
SBINDIR	    = $(PREFIX)/sbin

.PHONY:     clean
all:        $(PROG)
$(PROG):    $(OBJS)
	      $(CC) $(CFLAGS) $(EXTRA_LDFLAGS) $(LDFLAGS)  -o $(PROG) $(OBJS) -lusb

clean:
	      $(RM) ./usb_modeswitch

install: all
	mkdir -p $(SBINDIR)
	install ./usb_modeswitch $(SBINDIR)
	mkdir -p $(ETCDIR)
	install ./usb_modeswitch.conf $(ETCDIR)

uninstall: clean
	$(RM) /usr/sbin/usb_modeswitch
	$(RM) /etc/usb_modeswitch.conf

.PHONY:     clean install uninstall
