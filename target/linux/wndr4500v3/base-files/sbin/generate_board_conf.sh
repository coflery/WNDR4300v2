#!/bin/sh

CONFIG=/bin/config
board_hw_id="$(/sbin/artmtd -r board_hw_id | cut -f 2 -d ":" | cut -f 7 -d "+")"

/sbin/artmtd -r region
firmware_region=`cat /tmp/firmware_region | awk '{print $1}'`

#When board_model_id on HW board data area is WNDR4500v3
if [ "$board_hw_id" = "5508012173" ];then
	echo "WNDR4500v3" > /tmp/module_name
	echo "WNDR4500v3" > /tmp/hardware_version

	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set wan_hostname="WNDR4500v3"
		/bin/config set netbiosname="WNDR4500v3"
		/bin/config set upnp_serverName="ReadyDLNA: WNDR4500v3"
	fi

	/bin/config set bridge_netbiosname="WNDR4500v3"
	/bin/config set ap_netbiosname="WNDR4500v3"

	# minidlna modelname
	/bin/config set minidlna_modelname="Windows Media Connect compatible (NETGEAR WNDR4500v3)"	

	# miniupnp configure
	/bin/config set miniupnp_devupc="606449084528"
	/bin/config set miniupnp_friendlyname="NETGEAR WNDR4500v3 Wireless Router"
	/bin/config set miniupnp_modelname="RangeMax N900 Wireless Router"
	/bin/config set miniupnp_modelnumber="WNDR4500v3"
	/bin/config set miniupnp_modelurl="http://support.netgear.com/product/wndr4500v3"
	/bin/config set miniupnp_modeldescription="NETGEAR WNDR4500v3 RangeMax N900 Wireless Router"
	/bin/config set miniupnp_pnpx_hwid="VEN_01f2&amp;DEV_0009&amp;REV_03 VEN_01f2&amp;DEV_8000&amp;SUBSYS_01&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;REV_01 VEN_0033&amp;DEV_0008&amp;REV_01"

	#difference in net-cgi
	/bin/config set cgi_module_id="WNDR4500v3"
	/bin/config set cgi_ctl_mod="wndr4500v3"
	/bin/config set cgi_netgear_download="1"
	/bin/config set cgi_mode_2="216"
	/bin/config set cgi_mode_3="450"
	
	#difference in chainmask
	/bin/config set wl_chainmask=7
	/bin/config set wla_chainmask=7

	# madwifi_scripts
	ATH_TMP=/tmp/etc/ath
	ATH_ORI=/etc/ath.orig
	[ ! -d $ATH_TMP ] && mkdir -p $ATH_TMP && cp -a $ATH_ORI/* $ATH_TMP
	sed -i 's/wsc_manufactuer=.*/wsc_manufactuer="NTGR"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_name=.*/wsc_model_name="WNDR4500"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_number=.*/wsc_model_number="V3"/g' $ATH_TMP/board.conf

	# preset hw_board_type in kernel
	echo "WNDR4500v3" > /proc/hw_board_type

	# lld2d 
	cp /etc/wndr4500v3_icon.ico /tmp/icon.ico
	cp /etc/wndr4500v3_large.ico /tmp/large.ico
fi

#When board_model_id on HW board data area is WNDR4300v2
if [ "$board_hw_id" = "5508012175" ];then
	echo "WNDR4300v2" > /tmp/module_name
	echo "WNDR4300v2" > /tmp/hardware_version

	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set netbiosname="WNDR4300v2"
		/bin/config set wan_hostname="WNDR4300v2"
		/bin/config set upnp_serverName="ReadyDLNA: WNDR4300v2"
	fi

	/bin/config set bridge_netbiosname="WNDR4300v2"
	/bin/config set ap_netbiosname="WNDR4300v2"

	# minidlna modelname
	/bin/config set minidlna_modelname="Windows Media Connect compatible (NETGEAR WNDR4300v2)"

	# miniupnp configure
	/bin/config set miniupnp_devupc="606449084528"
	/bin/config set miniupnp_friendlyname="NETGEAR WNDR4300v2 Wireless Router"
	/bin/config set miniupnp_modelname="RangeMax N750 Wireless Router"
	/bin/config set miniupnp_modelnumber="WNDR4300v2"
	/bin/config set miniupnp_modelurl="http://support.netgear.com/product/wndr4300v2"
	/bin/config set miniupnp_modeldescription="NETGEAR WNDR4300v2 RangeMax N750 Wireless Router"
	/bin/config set miniupnp_pnpx_hwid="VEN_01f2&amp;DEV_000c&amp;REV_02 VEN_01f2&amp;DEV_8000&amp;SUBSYS_01&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;REV_01 VEN_0033&amp;DEV_0008&amp;REV_01"

	#differnece in net-cgi
	/bin/config set cgi_module_id="WNDR4300v2"
	/bin/config set cgi_ctl_mod="wndr4300v2"

	if [ "x$firmware_region" = "xRU" -o "x$firmware_region" = "xPR" ] ;then
		/bin/config set cgi_netgear_download="1"
	else
		/bin/config set cgi_netgear_download="0"
	fi

	/bin/config set cgi_mode_2="130"
	/bin/config set cgi_mode_3="300"

	#difference in chainmask
	/bin/config set wl_chainmask=3
	/bin/config set wla_chainmask=7

	# madwifi_scripts
	ATH_TMP=/tmp/etc/ath
	ATH_ORI=/etc/ath.orig
	[ -d $ATH_TMP ] || mkdir -p $ATH_TMP && cp -a $ATH_ORI/* $ATH_TMP
	sed -i 's/wsc_manufactuer=.*/wsc_manufactuer="NTGR"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_name=.*/wsc_model_name="WNDR4300"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_number=.*/wsc_model_number="V2"/g' $ATH_TMP/board.conf

	# preset hw_board_type in kernel
	echo "WNDR4300v2" > /proc/hw_board_type

	# lld2d
	cp /etc/wndr4300v2_icon.ico /tmp/icon.ico
	cp /etc/wndr4300v2_large.ico /tmp/large.ico
fi

#When board_model_id on HW board data area is WNDR4320
if [ "$board_hw_id" = "5508012183" ];then
	echo "WNDR4520" > /tmp/module_name
	echo "WNDR4520" > /tmp/hardware_version

	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set netbiosname="WNDR4520"
		/bin/config set wan_hostname="WNDR4520"
		/bin/config set upnp_serverName="ReadyDLNA: WNDR4520"
	fi

	/bin/config set bridge_netbiosname="WNDR4520"
	/bin/config set ap_netbiosname="WNDR4520"

	# minidlna modelname
	/bin/config set minidlna_modelname="Windows Media Connect compatible (NETGEAR WNDR4520)"

	# miniupnp configure
	/bin/config set miniupnp_devupc="606449084528"
	/bin/config set miniupnp_friendlyname="NETGEAR WNDR4520 Wireless Router"
	/bin/config set miniupnp_modelname="RangeMax N900 Wireless Router"
	/bin/config set miniupnp_modelnumber="WNDR4520"
	/bin/config set miniupnp_modelurl="http://support.netgear.com/product/wndr4520"
	/bin/config set miniupnp_modeldescription="NETGEAR WNDR4520 RangeMax N900 Wireless Router"
	/bin/config set miniupnp_pnpx_hwid="VEN_01f2&amp;DEV_001a&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;SUBSYS_01&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;REV_01 VEN_0033&amp;DEV_0008&amp;REV_01"

	#differnece in net-cgi
	/bin/config set cgi_module_id="WNDR4520"
	/bin/config set cgi_ctl_mod="wndr4520"
	/bin/config set cgi_netgear_download="1"
	/bin/config set cgi_mode_2="216"
	/bin/config set cgi_mode_3="450"

	#difference in chainmask
	/bin/config set wl_chainmask=7
	/bin/config set wla_chainmask=7

	# madwifi_scripts
	ATH_TMP=/tmp/etc/ath
	ATH_ORI=/etc/ath.orig
	[ -d $ATH_TMP ] || mkdir -p $ATH_TMP && cp -a $ATH_ORI/* $ATH_TMP
	sed -i 's/wsc_manufactuer=.*/wsc_manufactuer="NTGR"/g' $ATH_TMP/board.conf	
	sed -i 's/wsc_model_name=.*/wsc_model_name="WNDR4520"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_number=.*/wsc_model_number="V1"/g' $ATH_TMP/board.conf

	# preset hw_board_type in kernel
	echo "WNDR4520" > /proc/hw_board_type

	# lld2d
	cp /etc/wndr4520_icon.ico /tmp/icon.ico
	cp /etc/wndr4520_large.ico /tmp/large.ico
fi
