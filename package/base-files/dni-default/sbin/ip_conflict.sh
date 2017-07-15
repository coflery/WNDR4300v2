#!/bin/sh
nvram="/usr/sbin/nvram"

set_lan() #$1 is is the first 3 field of lan ip
{	
	echo -n "1" > /tmp/wan_lan_ip_conflict	
	
	ip=`$nvram get lan_ipaddr | sed 's/\([0-9]*.[0-9]*.[0-9]*\).*/\1/'`	

	for file in `ls /tmp/configs | grep forwarding | grep -v ^size`
	do
		new_info=`sed "s/[0-9]*\.[0-9]*\.[0-9]*/$ip/" /tmp/configs/$file`
		$nvram set $file="$new_info"
	done
	for file in `ls /tmp/configs | grep triggering | grep -v ^size`
	do
		new_info=`sed "s/[0-9]*\.[0-9]*\.[0-9]*/$ip/" /tmp/configs/$file`
		$nvram set $file="$new_info"
	done
	for file in `ls /tmp/configs | grep reservation | grep -v ^size`
	do
		new_info=`sed "s/[0-9]*\.[0-9]*\.[0-9]*/$ip/" /tmp/configs/$file`
		$nvram set $file="$new_info"
	done
	for file in `ls /tmp/configs | grep block_services | grep -v ^size`
	do
		new_info=`sed "s/[0-9]*\.[0-9]*\.[0-9]*/$ip/g" /tmp/configs/$file`
		$nvram set $file="$new_info"
	done
	new_info=`$nvram get dmz_ipaddr | sed "s/[0-9]*\.[0-9]*\.[0-9]*/$ip/"`
	$nvram set dmz_ipaddr="$new_info"
	new_info=`$nvram get block_trustedip | sed "s/[0-9]*\.[0-9]*\.[0-9]*/$ip/"`
	$nvram set block_trustedip="$new_info"	
	
	if [ -f /tmp/static_conflict ]; then
		rm -rf /tmp/static_conflict
		/sbin/cmdlan start
	else
		/www/cgi-bin/firewall.sh stop
		/sbin/cmdlan restart
		/www/cgi-bin/firewall.sh start
		$nvram commit
	fi
}

case "$1" in
        start)
			set_lan 
        	;;
        stop)
			#echo 1 > /tmp/configs/dns_hijack
			#echo 1 >/tmp/wan_lan_ip_conflict
        	;;
        *)
        	echo "Usage: /sbin/ip_conflict.sh start|stop"
        	;;
esac
