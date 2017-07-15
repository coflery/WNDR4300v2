#!/bin/sh

LOG_FILE=/var/log/messages

length=$(wc -l $LOG_FILE | awk '{ print $1 }')
if [ "$length" -gt 256 ]; then
	start_len=$((length - 256))
	sed -i "1,$start_len d" $LOG_FILE
fi
