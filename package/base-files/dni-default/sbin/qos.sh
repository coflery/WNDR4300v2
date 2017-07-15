#!/bin/sh
TC="/usr/sbin/tc"
IPTABLES="/usr/sbin/iptables"
WAN_IF="$(/usr/sbin/nvram get wan_ifname)"
# MANGLE_ADD="$IPTABLES -t mangle -A"
MANGLE_ADD="$IPTABLES -t mangle -A qos_chain"
FILTER_ADD="$TC filter add dev $WAN_IF"

start(){

echo -n > /tmp/configs/list_of_qos
i=1
while cat /tmp/configs/qos_list$i >> /tmp/configs/list_of_qos ;
do
       	i=$(($i + 1))
       	echo >> /tmp/configs/list_of_qos
done

if [ "x$WAN_IF" != "x" ] && [ "$(/usr/sbin/nvram get qos_endis_on)" = "1" ] && [ $i -gt 1 ]; then
	if [ $i -lt 5 ]; then
		BURST=12k
	elif [ $i -lt 10 ]; then
		BURST=11k
	elif [ $i -lt 15 ]; then
		BURST=10k
	elif [ $i -lt 20 ]; then
		BURST=8k
	elif [ $i -lt 25 ]; then
		BURST=6k
	elif [ $i -lt 30 ]; then
		BURST=5k
	else
		BURST=4k
	fi

	$TC qdisc add dev $WAN_IF root handle 1: htb default 2
	$TC class add dev $WAN_IF parent 1: classid 1:2 htb rate 100mbit burst $BURST
        $TC qdisc add dev $WAN_IF parent 1:2 handle 2: prio bands 4 priomap 2 3 3 3 2 3 1 1 2 2 2 2 2 2 2 2
	$TC qdisc add dev $WAN_IF parent 2:1 handle 10: sfq
	$TC qdisc add dev $WAN_IF parent 2:2 handle 20: sfq
	$TC qdisc add dev $WAN_IF parent 2:3 handle 30: sfq
	$TC qdisc add dev $WAN_IF parent 2:4 handle 40: sfq

	$FILTER_ADD parent 2:0 protocol ip prio 1 handle 1 fw classid 2:1
	$FILTER_ADD parent 2:0 protocol ip prio 2 handle 2 fw classid 2:2
	$FILTER_ADD parent 2:0 protocol ip prio 3 handle 3 fw classid 2:3
	$FILTER_ADD parent 2:0 protocol ip prio 4 handle 4 fw classid 2:4	

        $FILTER_ADD parent 2:0 protocol ip prio 5 u32 match ip tos 0x00 0xff flowid 2:3
        $FILTER_ADD parent 2:0 protocol ip prio 6 u32 match ip tos 0xc0 0xfc flowid 2:1
        $FILTER_ADD parent 2:0 protocol ip prio 7 u32 match ip tos 0x80 0xfc flowid 2:2
        $FILTER_ADD parent 2:0 protocol ip prio 5 u32 match ip tos 0x40 0xfc flowid 2:3
        $FILTER_ADD parent 2:0 protocol ip prio 8 u32 match ip tos 0x00 0xfc flowid 2:4

	$IPTABLES -t mangle -N qos_chain
	$IPTABLES -t mangle -A PREROUTING -j qos_chain

	while read LINE
	do
		case $(echo $LINE | awk '{print $2}') in
			0|1)
			START=$(echo $LINE | awk '{print $6}')
			END=$(echo $LINE | awk '{print $7}')
			RANGE=$(echo $START $END | awk -F"[ ,]" '{for(i=1; i<=NF/2; i++){j=i+NF/2; print $i, $j}}')
			PRIO=$(($(echo $LINE | awk '{print $4}') + 1))
			PROTO=$(echo $LINE | awk '{print $5}')
			if [ "x$(echo $PROTO | awk -F/ '{print $2}')" != "x" ]; then
				echo "$RANGE" | while read a b
				do
				$MANGLE_ADD -p tcp -m tcp --dport $a:$b -j MARK --set-mark $PRIO 
				#$MANGLE_ADD -p tcp -m tcp --dport $a:$b -j RETURN
				$MANGLE_ADD -p tcp -m tcp --sport $a:$b -j MARK --set-mark $PRIO
				#$MANGLE_ADD -p tcp -m tcp --sport $a:$b -j RETURN
				$MANGLE_ADD -p udp -m udp --dport $a:$b -j MARK --set-mark $PRIO
				#$MANGLE_ADD -p udp -m udp --dport $a:$b -j RETURN
				$MANGLE_ADD -p udp -m udp --sport $a:$b -j MARK --set-mark $PRIO
				#$MANGLE_ADD -p udp -m udp --sport $a:$b -j RETURN
				done 
			else
				if [ "x$PROTO" = "xTCP" ]; then
					PROTO="tcp"
				elif [ "x$PROTO" = "xUDP" ]; then
					PROTO="udp"
				fi

				echo "$RANGE" | while read a b
				do
				$MANGLE_ADD -p $PROTO -m $PROTO --dport $a:$b -j MARK --set-mark $PRIO
				#$MANGLE_ADD -p $PROTO -m $PROTO --dport $a:$b -j RETURN
				$MANGLE_ADD -p $PROTO -m $PROTO --sport $a:$b -j MARK --set-mark $PRIO
				#$MANGLE_ADD -p $PROTO -m $PROTO --sport $a:$b -j RETURN
				done
			fi
			;;
			2)
				LPORT=$(echo $LINE | awk '{print $3}')
				PORT=$((4 - $LPORT))
				LPRIO=$(echo $LINE | awk '{print $4}')
				PRIO=$(($LPRIO + 1))
				echo $PORT$PRIO > /proc/lan_prio_map
			;;
			3)
			MAC=$(echo $LINE | awk '{print $9}')
			PRIO=$(($(echo $LINE | awk '{print $4}') + 1 ))
			$MANGLE_ADD -m mac --mac-source $MAC -j MARK --set-mark $PRIO
			#$MANGLE_ADD -m mac --mac-source $MAC -j RETURN
			;;
		esac
	done < /tmp/configs/list_of_qos
fi
}

stop(){
	$TC qdisc del dev $WAN_IF root
#	$TC qdisc del dev $LAN_IFACE root
	$IPTABLES -t mangle -D PREROUTING -j qos_chain
	$IPTABLES -t mangle -F qos_chain
	$IPTABLES -t mangle -X qos_chain
	for i in 0 1 2 3
	do
		echo $i\0 > /proc/lan_prio_map
	done
}

status(){
	echo "show qdisc ............ "
	$TC -d -s qdisc
#	echo "show filter ............ "
#		$TC -d -s filter ls dev $WAN_IF
	echo "show class ............ "
	if [ "x$WAN_IF" != "x" ];then
		$TC -d -s class ls dev $WAN_IF
	fi
}
								 
case "$1" in
	start)
	start
	;;
	stop)
	stop
	;;
	restart)
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

