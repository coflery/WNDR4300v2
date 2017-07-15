#!/bin/sh
# (C) 2008 openwrt.org

. /etc/functions.sh
STATUS=$1
hnat_enable() {
	echo "***************enable hnat******************" > /dev/console
	echo 1 > /proc/qca_switch/nf_athrs17_hnat
}

hnat_disable() {
	echo "***************disable hnat******************" > /dev/console
	echo 0 > /proc/qca_switch/nf_athrs17_hnat
}

hnat_restart() {
	echo "****************restart hnat*****************" > /dev/console
	echo 0 > /proc/qca_switch/nf_athrs17_hnat
	sleep 5
	echo 1 > /proc/qca_switch/nf_athrs17_hnat
	sleep 1
}

hnat_help() {
	echo "********usage: hnat.sh enable/disable******************"
}

case $STATUS in
    enable)
        hnat_enable
        ;;
    disable)
        hnat_disable
        ;;
    restart)
        hnat_restart
	;;
    *)
        hnat_help
	;;
esac
