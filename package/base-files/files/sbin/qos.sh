#!/bin/sh
TC="/usr/sbin/tc"
IPTABLES="/usr/sbin/iptables"
NVRAM="/bin/config"
ECHO="/bin/echo"
WAN_IF="$($NVRAM get wan_ifname)"
LAN_IF="$($NVRAM get lan_ifname)"
WAN_PROTO="$($NVRAM get wan_proto)"
FILTER_ADD="$TC filter add dev $WAN_IF"
UPRATE="$($NVRAM get qos_uprate)"
QoS_ENABLE="$($NVRAM get qos_endis_on)"
BANDCTL="$($NVRAM get qos_threshold)"
WAN_SPEED=`cat /tmp/WAN_status | cut -f 1 -d 'M'`
QoS_MODE="$($NVRAM get qos_mode)"
BANDWIDTH="$($NVRAM get qos_bandwidth)"
FAST_STATUS="$($NVRAM get qos_fast_status)"
WPS_SWITCH="$($NVRAM get quick_wps_fastlane)"


g_enabled="$($NVRAM get endis_wl_radio)"
g_sectype="$($NVRAM get wl_sectype)"

start(){
	if [ "x$QoS_ENABLE" != "x1" ]; then
		$ECHO -n 0:$BANDCTL > /proc/MFS
		return
	fi

	if [ $UPRATE -le 0 ] || [ $UPRATE -gt 1000000 ]; then
		UPRATE=1000000
	fi

	dni_qos --MFS $UPRATE:1 --dni_qos_if $WAN_IF

	if [ "x$QoS_MODE" = "x1" ]; then
		if [ "x$WPS_SWITCH" = "xfastlane" ];then
			if [ "x$FAST_STATUS" = "x1" ];then
				/sbin/ledcontrol -n wps -c green -s on
			else 
				/sbin/ledcontrol -n wps -c green -s off
			fi
		else
			if [ "x$g_enabled" = "x1" -a "x$g_sectype" != "x1" ]; then
				/sbin/ledcontrol -n wps -c green -s on
			else
				/sbin/ledcontrol -n wps -c green -s off
			fi
		fi

		dni_lan_qos --dni_lan_qos_if $LAN_IF --qos_bdw_res "$UPRATE:$BANDWIDTH" --lan_qos_enable $FAST_STATUS
	else
		if [ "x$WPS_SWITCH" = "xfastlane" ];then
			/sbin/ledcontrol -n wps -c green -s off
		fi

		dni_lan_qos --dni_lan_qos_if $LAN_IF --qos_bdw_res "1000000:$BANDWIDTH" --lan_qos_enable 0
	fi

}

stop(){
	dni_qos --MFS "0:$BANDCTL"

	if [ "x$WPS_SWITCH" = "xfastlane" ];then
		/sbin/ledcontrol -n wps -c green -s off
	fi

	dni_lan_qos --dni_lan_qos_if $LAN_IF --qos_bdw_res "1000000:$BANDWIDTH" --lan_qos_enable 0
}

status(){
	$IPTABLES -t mangle -nvL
}
								 
case "$1" in
	stop)
	stop
	;;
	start | restart )
	stop
	start
	;;
	status)
	status
	;;
	*)
	echo $"Usage:$0 {start|stop|restart|status}"
	exit 1
esac

