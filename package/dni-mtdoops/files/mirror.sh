# !/bin/sh

port(){
if [ "$2" = "on" ];then
	#mirror off first
	ethreg -i phy0 0x620=$(( $(ethreg -i phy0 0x620 |cut -d ' ' -f 5) | 0xf0 ))
	ethreg -i phy0 0x69c=$(( $(ethreg -i phy0 0x69c |cut -d ' ' -f 5) ^ (0x1 << 25) ))
	ethreg -i phy0 0x99c=$(( $(ethreg -i phy0 0x99c |cut -d ' ' -f 5) ^ (0x1 << 16) ))
	#set mirror port
	num=$1
	ethreg -i phy0 0x620=$(( $(ethreg -i phy0 0x620 |cut -d ' ' -f 5) ^ ((0xf - $num) << 4) ))
	#set ingress port
	ethreg -i phy0 0x69c=$(( $(ethreg -i phy0 0x69c |cut -d ' ' -f 5) | (0x1 << 25) ))
	#set egress port
	ethreg -i phy0 0x99c=$(( $(ethreg -i phy0 0x99c |cut -d ' ' -f 5) | (0x1 << 16) ))
else
	#unset mirror port
	ethreg -i phy0 0x620=$(( $(ethreg -i phy0 0x620 |cut -d ' ' -f 5) | 0xf0 ))
	#unset ingress port
	ethreg -i phy0 0x69c=$(( $(ethreg -i phy0 0x69c |cut -d ' ' -f 5) ^ (0x1 << 25) ))
	#unset egress port
	ethreg -i phy0 0x99c=$(( $(ethreg -i phy0 0x99c |cut -d ' ' -f 5) ^ (0x1 << 16) ))
fi
} 

if [ "$2" != "on" -a "$2" != "off" ];then
	echo "usage: mirror.sh <port> <status>"
	echo "example: mirror.sh 1 on"
	return
fi
port $1 $2

