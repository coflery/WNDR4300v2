#!/bin/sh

REALM=`/bin/cat /module_name | sed 's/\n//g'`
UHTTPD_BIN="/usr/sbin/uhttpd"
PX5G_BIN="/usr/sbin/px5g"
CFG="/bin/config get"

uhttpd_stop()
{
	kill -9 $(pidof uhttpd)
}

uhttpd_start()
{
	/sbin/artmtd -r region

        [ -x "$PX5G_BIN" ] && {
                $PX5G_BIN selfsigned -der \
                        -days ${days:-730} -newkey rsa:${bits:-1024} -keyout "/tmp/uhttpd.key" -out "/tmp/uhttpd.crt" \
                        -subj /C=${country:-DE}/ST=${state:-Saxony}/L=${location:-Leipzig}/CN=${commonname:-OpenWrt}
        } || {
                echo "WARNING: the specified certificate and key" \
                        "files do not exist and the px5g generator" \
                        "is not available, skipping SSL setup."
        }

        $UHTTPD_BIN -h /www -r ${REALM}  -x /cgi-bin -t 60 -p 0.0.0.0:80 -C /tmp/uhttpd.crt -K /tmp/uhttpd.key -s 0.0.0.0:443
}

reservation_table_init()
{
	rm -rf /tmp/reservation_table

	prefix="reservation"

	for i in `seq 64`; do
		value=`$CFG $prefix$i`	
		if [ "x$value" = "x" ]; then
			break
		else
			echo "$value" >> /tmp/reservation_table
		fi
	done
}

case "$1" in
	stop)
		uhttpd_stop
	;;
	start)
		uhttpd_start
		reservation_table_init
	;;
	restart)
		uhttpd_stop
		uhttpd_start
		reservation_table_init
	;;
	*)
		logger -- "usage: $0 start|stop|restart"
	;;
esac

