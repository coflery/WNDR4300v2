# for ap152-wndr4500v3 (s17 switch) :
#    sw port0 -> cup port
#    sw port1 (phy0) -> LAN1
#    sw port2 (phy1) -> LAN2
#    sw port3 (phy2) -> LAN3
#    sw port4 (phy3) -> LAN4
#    sw port5 (phy4) -> WAN

rawif=phy0

get_reg() # $1: reg_addr
{
	ethreg -i $rawif $1 | awk '{print $5}'
}

set_reg() # $1: reg_addr, $2: value, $3: mask
{
	if [ $# == 2 ]; then
		ethreg -i $rawif $1=$2
		return
	fi

	local v0=$(get_reg $1)
	local v_value=$(($2 & $3))
	local v_clear=$(($3 ^ 0xffffffff))
	ethreg -i $rawif $1=$(($v0 & $v_clear | $v_value))
}

sw_init()
{
	set_reg 0x620 0x080004f0   # GLOBAL_FW_CTRL0
	set_reg 0x660 0x0014017e   # PORT0_LOOKUP_CTRL
	set_reg 0x66c 0x0014017d   # PORT1_LOOKUP_CTRL
	set_reg 0x678 0x0014017b   # PORT2_LOOKUP_CTRL
	set_reg 0x684 0x00140177   # PORT3_LOOKUP_CTRL
	set_reg 0x690 0x0014016f   # PORT4_LOOKUP_CTRL
	set_reg 0x69c 0x0014015f   # PORT5_LOOKUP_CTRL
	set_reg 0xc00 0x00000002   # PKT_EDIT_CTRL
}

# Enable IGMP SNOOPING on LAN (0,2,3,4,5) ports
sw_enable_igmp_snooping() #
{
	set_reg 0x624 0x7f000000 0x7f000000  # IGMP_JOIN_LEAVE_DP : flood IGMP/MLD
	set_reg 0x210 0x06060606 0x06060606  # IGMP_LEAVE_EN & IGMP_JOIN_EN (0,1,2,3 ports)
	set_reg 0x214 0x01000006 0x01000006  # IGMP_V3_EN,
	                                     # IGMP_LEAVE_EN & IGMP_JOIN_EN (4 port)
}

sw_disable_igmp_snooping() #
{
	set_reg 0x624 0x7f000000 0x7f000000  # IGMP_JOIN_LEAVE_DP : flood IGMP/MLD
	set_reg 0x210 0x00000000 0x06060606  # IGMP_LEAVE_EN & IGMP_JOIN_EN (0,1,2,3 ports)
	set_reg 0x214 0x00000000 0x01000006  # IGMP_V3_EN,
	                                     # IGMP_LEAVE_EN & IGMP_JOIN_EN (4 port)
}

sw_port_type()
# $1: switch port
# $2: iptv_port_mask
#     eg. 0x1 means LAN1 is iptv port, 0x9 meas LAN1 & LAN4 are iptv ports, ...
# return : cpu/wan/iptv/lan
{
	local i_mask
	local iptv_port_mask

	case "$1" in
	0)
		echo "cpu"
		;;
	5)
		echo "wan"
		;;
	1|2|3|4)
		i_mask=$((1 << $(($1 - 1)) ))
		[ $# == 2 ] && iptv_port_mask=$2 || iptv_port_mask=0
		[ $(( $iptv_port_mask & $i_mask )) == 0 ] && echo "lan" || echo "iptv"
		;;
	esac
}

sw_config_vlan()
# $1: iptv ports, 
#     eg. 0x1 means LAN1 is iptv port, 0x9 meas LAN1 & LAN4 are iptv ports, ...
{
	local iptv_port_mask=$(($1 & 0xf))
	local port_vlan_ctrl0 
	local port_vlan_ctrl1 
	local port_vlan_ctrl0_val
	local port_vlan_ctrl1_val
	local v_610_vlan1
	local v_610_vlan2
	local v_610_vlan1_p=0
	local v_610_vlan2_p=0

	for i in 0 1 2 3 4 5; do
		case "$(sw_port_type $i $iptv_port_mask)" in
		cpu)
			port_vlan_ctrl0_val=0x00010001
			port_vlan_ctrl1_val=0x00002040
			v_610_vlan1_p=$(($v_610_vlan1_p | 0x2 << $((2 * $i)) ))
			v_610_vlan2_p=$(($v_610_vlan2_p | 0x2 << $((2 * $i)) ))
		;;
		wan|iptv)
			port_vlan_ctrl0_val=0x00020001
			port_vlan_ctrl1_val=0x00001040
			v_610_vlan1_p=$(($v_610_vlan1_p | 0x3 << $((2 * $i)) ))
			v_610_vlan2_p=$(($v_610_vlan2_p | 0x1 << $((2 * $i)) ))
		;;
		lan)
			port_vlan_ctrl0_val=0x00010001
			port_vlan_ctrl1_val=0x00001040
			v_610_vlan1_p=$(($v_610_vlan1_p | 0x1 << $((2 * $i)) ))
			v_610_vlan2_p=$(($v_610_vlan2_p | 0x3 << $((2 * $i)) ))
		;;
		esac

		port_vlan_ctrl0=$((0x420 + $((8 * $i)) ))
		port_vlan_ctrl1=$((0x424 + $((8 * $i)) ))
		set_reg $port_vlan_ctrl0 $port_vlan_ctrl0_val
		set_reg $port_vlan_ctrl1 $port_vlan_ctrl1_val
	done

	v_610_vlan1=$((0x001b0000 | $(($v_610_vlan1_p << 4)) ))
	v_610_vlan2=$((0x001b0000 | $(($v_610_vlan2_p << 4)) ))

	# flush all vlan table
	set_reg 0x614 0x80000001
	# add vlan 1 in vlan table
	set_reg 0x610 $v_610_vlan1
	set_reg 0x614 0x80010002
	# add vlan 2 in vlan table
	set_reg 0x610 $v_610_vlan2
	set_reg 0x614 0x80020002
}


sw_acl_rule()
{
    set_reg 0x030 0x80000302
    #rule
    set_reg 0x404 0x5e7ffffa #DA:byte 0-5:   destination mac addr
    set_reg 0x408 0x00000100 #SA DA
    set_reg 0x40c 0x00000000 #SA
    set_reg 0x410 0x00000001 #vlan1
    set_reg 0x414 0x00000001 #cpu port
    set_reg 0x400 0x80000020
    #mask
    set_reg 0x404 0xffffffff #byte 0-5:   destination mac addr mask
    set_reg 0x408 0x0000ffff
    set_reg 0x40c 0x00000000
    set_reg 0x410 0x00000fff
    set_reg 0x414 0x000000f9 #start&end, untaged frame, mac rule
    set_reg 0x400 0x80000120
    #result
    set_reg 0x404 0x00000000
    set_reg 0x408 0xc0000000
    set_reg 0x40c 0x00000013
    set_reg 0x410 0x00000000
    set_reg 0x414 0x00000000
    set_reg 0x400 0x80000220

    #IPv6 multicast cpu->lan
    #rule
    set_reg 0x404 0x00000000 #MAC DA: 33:33:XX:XX:XX:XX
    set_reg 0x408 0x00003333 #SA DA
    set_reg 0x40c 0x00000000 #SA
    set_reg 0x410 0x00000001 # vlan 1
    set_reg 0x414 0x00000001 #Source port: cpu port
    set_reg 0x400 0x80000021 #add mac rule
    #mask
    set_reg 0x404 0x00000000 #byte 0-5:   destination mac addr mask
    set_reg 0x408 0x0000ffff
    set_reg 0x40c 0x00000000
    set_reg 0x410 0x00000fff
    set_reg 0x414 0x000000f9 #start&end, taged frame, mac rule
    set_reg 0x400 0x80000121
    #result
    set_reg 0x404 0x00000000
    set_reg 0x408 0xc0000000
    set_reg 0x40c 0x00000013
    set_reg 0x410 0x00000000
    set_reg 0x414 0x00000000
    set_reg 0x400 0x80000221


    #IPv6 multicast lan->cpu
    #rule
    set_reg 0x404 0x00000000 #MAC DA: 33:33:XX:XX:XX:XX
    set_reg 0x408 0x00003333 #SA DA
    set_reg 0x40c 0x00000000 #SA
    set_reg 0x410 0x00000000 #
    set_reg 0x414 0x0000001e #Source port: lan port
    set_reg 0x400 0x80000022 #add mac rule
    #mask
    set_reg 0x404 0x00000000 #byte 0-5:   destination mac addr mask
    set_reg 0x408 0x0000ffff
    set_reg 0x40c 0x00000000
    set_reg 0x410 0x00000000
    set_reg 0x414 0x000000c9 #start&end, untaged frame, mac rule
    set_reg 0x400 0x80000122
    #result
    set_reg 0x404 0x00000000
    set_reg 0x408 0xe0000000 #to cpu and lan
    set_reg 0x40c 0x00000013 #to cpu and lan
    set_reg 0x410 0x00000000
    set_reg 0x414 0x00000000
    set_reg 0x400 0x80000222

    #IPv6 multicast  cpu->wan
    #rule
    set_reg 0x404 0x00000000 #MAC DA: 33:33:XX:XX:XX:XX
    set_reg 0x408 0x00003333 #SA DA
    set_reg 0x40c 0x00000000 #SA
    set_reg 0x410 0x00000002 #vlan 2
    set_reg 0x414 0x00000001 #Source port: cpu port
    set_reg 0x400 0x80000023 #add mac rule
    #mask
    set_reg 0x404 0x00000000 #byte 0-5:   destination mac addr mask
    set_reg 0x408 0x0000ffff
    set_reg 0x40c 0x00000000
    set_reg 0x410 0x00000000
    set_reg 0x414 0x000000f9 #start&end, taged frame, mac rule
    set_reg 0x400 0x80000123
    #result
    set_reg 0x404 0x00000000
    set_reg 0x408 0x00000000
    set_reg 0x40c 0x00000014 #to wan
    set_reg 0x410 0x00000000
    set_reg 0x414 0x00000000
    set_reg 0x400 0x80000223

#    ethreg -i phy0 0x400=0x80000403
#	 ethreg -i phy0 0x400
#    ethreg -i phy0 0x404
#    ethreg -i phy0 0x408

}




