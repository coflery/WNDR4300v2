#!/bin/sh

PATH="$1"
EN="$2"
GREP="/bin/grep"
ECHO="/bin/echo"
SLEEP="/bin/sleep"

if [ "X$($ECHO $PATH | $GREP usb1)" != "X" ]; then
	ID=0
elif [ "X$($ECHO $PATH | $GREP usb2)" != "X" ]; then
	ID=1
fi

SID=$((ID+4))

if [ "$EN" = "on" ]; then
	$SLEEP 6
	/usr/sbin/usb_led_stop $SID 2> /dev/null
	/sbin/ledcontrol -n usb"$ID" -c green -s off
	/usr/sbin/dongle_led $ID 2> /dev/null & 
elif [ "$EN" = "off" ]; then
	SID=$((ID+4))
	/usr/sbin/usb_led_stop $SID 2> /dev/null
	/sbin/ledcontrol -n usb"$ID" -c green -s off
fi
