#! /bin/sh

while [ 1 ]
do
	#check if lost connection is 3G
	date >> /tmp/log_3g_reconnect
	[ `/bin/config get wan_proto` != "3g" ] && exit
	[ `/bin/grep -c usbserial_generic /proc/bus/usb/devices` -eq 0 ] && [ `/bin/grep -c cdc_acm /proc/bus/usb/devices` -eq 0 ] && exit
	echo "3G connection is lost and we need to make it reconnect ..." >> /tmp/log_3g_reconnect
	#reset usb port
	echo 0 > /proc/simple_config/usb_5v_en
	sleep 5
	echo 1 > /proc/simple_config/usb_5v_en
	sleep 10
	#do reconnect
	/usr/bin/nohup /etc/init.d/net-wan restart &
	#check result
	sleep 60
	[ -f /etc/ppp/ppp0-status ] && [ `cat /etc/ppp/ppp0-status` -eq 1 ] && echo "3G reconnection is completed and OK." >> /tmp/log_3g_reconnect && exit
done
