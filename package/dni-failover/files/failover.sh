#!/bin/sh
#set -x

FAILOVER_RESOLVE=/tmp/resolv_for_failover.conf

local proto interface ipaddr gw

config_dns() {
	while read line
	do	
		dns=$(echo $line | awk -F "nameserver " '{print $2}')

		if [ "x$dns" = "x" -o "x$gw" = "x" ]; then
			exit 1
		fi

		iptables -I INPUT -i $interface -j ACCEPT
		iptables -I OUTPUT -o $interface -j ACCEPT
		if [ "$proto" = "pptp" -o "$proto" = "l2tp" ]; then
			iptables -I INPUT -i eth1 -j ACCEPT
			iptables -I OUTPUT -o eth1 -j ACCEPT
		else
			route add $dns gw $gw
		fi
	done < $FAILOVER_RESOLVE

	route add $ipaddr gw $gw
}

remove_dns() {
	while read line
	do
		dns=$(echo $line | awk -F "nameserver " '{print $2}')
		if [ "x$dns" = "x" -o "x$gw" = "x" ]; then
			exit 1
		fi

		route del $dns
	done < $FAILOVER_RESOLVE

	route del $ipaddr

	iptables -D INPUT -i $interface -j ACCEPT
	iptables -D OUTPUT -o $interface -j ACCEPT
}

firewall_config() {
	if [ "$1" = "insert" ]; then
		iptables -I INPUT -i $2 -j ACCEPT
		iptables -I OUTPUT -o $2 -j ACCEPT
	else
		iptables -D INPUT -i $2 -j ACCEPT
		iptables -D OUTPUT -o $2 -j ACCEPT
	fi
}

do_config() {

	if [ "x$1" = "x" -o "$gw" = "x" ]; then
		exit 1
	fi
	iptables -I INPUT -i $interface -j ACCEPT
	iptables -I OUTPUT -o $interface -j ACCEPT
	route add $1 gw $gw
}

do_clean()
{
	if [ "x$1" = "x" ]; then
		exit 1
	fi
	route del $1
	iptables -D INPUT -i $interface -j ACCEPT
	iptables -D OUTPUT -o $interface -j ACCEPT

}

clean_up()
{
	local process
	local mobile_ifname=$(/bin/config get wan_mobile_ifname)

	if [ "$1" = "static" ]; then
		ifconfig eth1 0.0.0.0
	elif [ "$1" = "dhcp" ]; then
		process=$(ps | grep udhcpc | grep eth1 | cut -c 1-5)
		[ "x$process" != "x" ] && kill $process
	elif [ "$1" = "3g" ] && [ "$mobile_ifname" = "eth2" ]; then
		process=$(ps | grep udhcpc | grep eth2 | cut -c 1-5)
		[ "x$process" != "x" ] && kill $process
	elif [ "$1" = "3g" ]; then
		process=$(ps | grep pppd | grep "unit 1" | cut -c 1-5)
		[ "x$process" != "x" ] && kill $process
	else
		process=$(ps | grep pppd | grep "unit 1" | cut -c 1-5)
		[ "x$process" != "x" ] && kill $process
		process=$(ps | grep udhcpc | grep eth1 | cut -c 1-5)
		[ "x$process" != "x" ] && kill $process
	fi
}

if [ "$1" = "init_env" ]; then
	if [ "$2" != "3g" -a "$2" != "wimax" ]; then
		ifconfig eth1 down
		ifconfig eth1 up
	fi
	/bin/config set failover_detect_proto=$2
	/etc/failover/net-$2
elif [ "$1" = "config_fw" ]; then
	firewall_config $2 $3
elif [ "$1" = "do_config" ]; then
	mobile_ifname=$(/bin/config get wan_mobile_ifname)
	proto=$2
	if [ "$proto" = "static" ]; then
		interface=eth1
		gw=$(/bin/config get wan_gateway)
	elif [ "$proto" = "dhcp" ]; then
		interface=eth1
		gw=$(/bin/config get wan_dhcp_gateway)
	elif [ "$proto" = "3g" ] && [ "$mobile_ifname" = "eth2" ]; then
		interface=eth2
		gw=$(/bin/config get lte_dhcp_gateway)
	else
		interface=ppp1
		gw=$(ifconfig $interface | sed -ne 's/\(.*\)P-t-P:\([[:digit:].]*\)\(.*\)/\2/p')
	fi

	interface_exist=$(ifconfig | grep $interface)
	if [ "x$interface_exist" = "x" ]; then
		exit 1
	fi

	ipaddr=$(ifconfig $interface | sed -ne 's/\(.*\)inet addr:\([[:digit:].]*\)\(.*\)/\2/p')

	if [ "$3" = "dns_add" ]; then
		config_dns
	elif [ "$3" = "dns_del" ]; then
		remove_dns
	elif [ "$3" = "add" ]; then
		do_config $4
	elif [ "$3" = "remove" ]; then
		do_clean $4
	fi
elif [ "$1" = "clean_up" ]; then
	clean_up $2
fi

#set +x
