#!/bin/sh
append DRIVERS "madwifi11n"

madwifi11n_hostapd_setup_cfg() {
    local vif="$1"
    local driver="$2"
    local output_file="$3"
    local hostapd_cfg=

    config_get_bool wps "$vif" wps 0
    config_get ssid "$vif" ssid
    config_get enc "$vif" encryption

    case "$enc" in
        wpa2*|WPA2*|*PSK2*|*psk2*)
            wpa=2
            crypto="CCMP"
        ;;
        *mixed*)
            wpa=3
            crypto="CCMP TKIP"
        ;;
        *)
            wpa=1
            crypto="TKIP"
        ;;
    esac

    case "$enc" in
        none)
            wpa=0
            crypto=
            ;;
        *)
            config_get psk "$vif" key
	    if [ ${#psk} -eq 64 ]; then
		append hostapd_cfg "wpa_psk=$psk" "$N"
	    else
		append hostapd_cfg "wpa_passphrase=$psk" "$N"
	    fi
            append hostapd_cfg "wpa_key_mgmt=WPA-PSK" "$N"
            config_get wpa_group_rekey "$vif" wpa_group_rekey 3600
            append hostapd_cfg "wpa_group_rekey=$wpa_group_rekey" "$N"
            config_get wpa_gmk_rekey "$vif" wpa_gmk_rekey 3600
            append hostapd_cfg "wpa_gmk_rekey=$wpa_gmk_rekey" "$N"
            ;;
    esac
    if [ $wps -eq 1 ]; then
        case "$enc" in
            none|psk*|wpa*)
                append hostapd_cfg "wps_auth_type_flags=0x0023" "$N"
                ;;
        esac
        append hostapd_cfg "wps_version=0x10" "$N"
        append hostapd_cfg "wps_upnp_disable=0" "$N"
        append hostapd_cfg "wps_disable=0" "$N"
        append hostapd_cfg "wps_conn_type_flags=0x01" "$N"
        append hostapd_cfg "wps_config_methods=0x0086" "$N"
        config_get wps_configured "$vif" wps_configured 0
        append hostapd_cfg "wps_configured=$wps_configured" "$N"
        append hostapd_cfg "wps_rf_bands=0x03" "$N"
        append hostapd_cfg "wps_manufacturer=None" "$N"
        append hostapd_cfg "wps_model_name=None" "$N"
        append hostapd_cfg "wps_model_number=None" "$N"
        append hostapd_cfg "wps_serial_number=None" "$N"
        append hostapd_cfg "wps_friendly_name=FriendlyNameHere" "$N"
        append hostapd_cfg "wps_manufacturer_url=http://manufacturer.url.here" "$N"
        append hostapd_cfg "wps_model_description=Model description here" "$N"
        append hostapd_cfg "wps_model_url=http://model.url.here" "$N"
        append hostapd_cfg "wps_upc_string=upc string here" "$N"
        config_get wps_pin "$vif" wps_pin 12345670
        append hostapd_cfg "wps_default_pin=$wps_pin" "$N"
        append hostapd_cfg "wps_dev_category=6" "$N"
        append hostapd_cfg "wps_dev_sub_category=1" "$N"
        append hostapd_cfg "wps_dev_oui=0050f204" "$N"
        append hostapd_cfg "wps_dev_name=None" "$N"
        append hostapd_cfg "wps_os_version=0x00000001" "$N"
        append hostapd_cfg "wps_atheros_extension=0" "$N"
        config_get_bool wps_locked "$vif" wps_locked 0
        append hostapd_cfg "wps_ap_setup_locked=wps_locked" "$N"
        append hostapd_cfg "wps_upnp_ad_period=1800" "$N"
        append hostapd_cfg "wps_upnp_ad_ttl=4" "$N"
    else
        append hostapd_cfg "wps_disable=1" "$N"
        append hostapd_cfg "wps_upnp_disable=1" "$N"
        append hostapd_cfg "wpa_strict_rekey=1" "$N"
    fi
    cat > $output_file <<EOF

ignore_file_errors=1
logger_syslog=-1
logger_syslog_level=2
logger_stdout=-1
logger_stdout_level=2
debug=0
ctrl_interface=/var/run/hostapd
ctrl_interface_group=0
ssid=$ssid
dtim_period=2
max_num_sta=255
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wme_enabled=0
ieee8021x=0
eapol_version=2
eapol_key_index_workaround=0
eap_server=1
eap_user_file=/etc/wpa2/hostapd.eap_user
wpa=$wpa
${crypto:+wpa_pairwise=$crypto}
$hostapd_cfg
EOF

}

madwifi11n_hostapd_setup_vif() {
    local dev=$1
    local vifs=$2
    local topology_cfg=/tmp/hostapd-topology.conf

    [ -f /tmp/hostapd-topology.conf ] || {

        config_cb() {
            [ "$2" = "lan" ] && eval "bridge_if=br-$2"
        }
        config_load network
        [ -z "$bridge_if" ] && bridge_if=br0

        cat > $topology_cfg <<EOF
bridge $bridge_if
{
        ipaddress 192.168.1.2
        ipmask 255.255.255.0
        #new_interface
}
EOF
    }

cat >> $topology_cfg <<EOF
radio $dev
{
        ap
        {
EOF

    for vif in $vifs; do
        config_get ifname "$vif" ifname
        local hostapd_cfg=/tmp/hostapd-$ifname.conf
        madwifi11n_hostapd_setup_cfg $vif "madwifi" $hostapd_cfg
        sed -i "/#new_interface/a\        interface $ifname" $topology_cfg
        cat >> $topology_cfg <<EOF
                bss $ifname
                {
                        config $hostapd_cfg
                }
EOF
    done
    cat >> $topology_cfg <<EOF
        }
}
EOF
    # kill previous activated hostapd
    for pid in `pidof hostapd`; do
        grep "hostapd-topology" /proc/$pid/cmdline >/dev/null && \
            kill $pid
    done

    hostapd $topology_cfg -B
}

scan_madwifi11n() {
	local device="$1"
	local adhoc sta ap monitor mesh

	config_get vifs "$device" vifs
	for vif in $vifs; do
		config_get mode "$vif" mode
		case "$mode" in
			adhoc|sta|ap|monitor|mesh)
				append $mode "$vif"
			;;
			*) echo "$device($vif): Invalid mode, ignored."; continue;;
		esac
	done

	config_set "$device" vifs "${ap:+$ap }${adhoc:+$adhoc }${sta:+$sta }${monitor:+$monitor }${mesh:+$mesh}"
}

load_driver() {
    [ -n "$1" ] && {
        local drive_loaded=`cat /proc/modules | grep $1`
        [ -z "$drive_loaded" ] && insmod $1
    }
}

load_madwifi11n_driver() {
    load_driver adf
    load_driver asf
    load_driver ath_hal
    load_driver ath_rate_atheros
    load_driver ath_spectral
    load_driver ath_dev
    load_driver umac
    load_driver wlan_me
    load_driver ath_pktlog
}

unload_madwifi11n_driver() {
    # Don't need to unload driver so far.
    return 0;
}

enable_madwifi11n() {
    [ -f /sbin/update-wifi -a -f /sbin/wlan ] && {
        /sbin/wlan up
        return
    }
    load_madwifi11n_driver
    local device="$1"
    local hostapd_vifs=
    config_get channel "$device" channel
    config_get vifs "$device" vifs
    config_get txpower "$device" txpower
    config_get htmode "$device" htmode
    config_get hwmode "$device" hwmode
    config_get cwmmode "$device" cwmmode 1

    config_get haldbg "$device" haldbg 0
    iwpriv "$device" HALDbg $haldbg

    config_get athdbg "$device" athdbg 0
    iwpriv "$device" ATHDebug $athdbg

    echo "$hwmode" | grep -q -i 'G' && {
        band='g'
    } || {
        band='a'
    }

    local extoffset
    local chmode
    case "$htmode" in
        *40+)
            extoffset=1
            chmode=11n${band}ht40plus
            [ "$band" = "g" ] && forbiasauto=1
            ;;
        *40-)
            extoffset=-1
            chmode=11n${band}ht40minus
            [ "$band" = "g" ] && forbiasauto=1
            ;;
        *20)
            extoffset=
            chmode=11n${band}ht20
            [ "$band" = "g" ] && forbiasauto=1
            cwmmode=0
            ;;
        *)
            extoffset=
            chmode=11${band}
            cwmmode=0
            ;;
    esac

    [ -n "$forbiasauto" ] && iwpriv "$device" ForBiasAuto $forbiasauto

    config_get txqlen "$device" txqlen 1000
    iwpriv "$device" txqueuelen $txqlen

    [ auto = "$channel" ] && channel=0

    config_get ampdu "$device" ampdu 1
    iwpriv "$device" AMPDU $ampdu

    config_get ampduframs "$device" ampduframes 32
    iwpriv "$device" AMPDUFrames $ampduframs

    config_get ampdulim "$device" ampdulim 50000
    iwpriv "$device" AMPDULim $ampdulim

    config_get txchainmask "$device" txchainmask
    [ -n "$txchainmask" ] && iwpriv "$device" txchainmask $txchainmask

    config_get rxchainmask "$device" rxchainmask
    [ -n "$rxchainmask" ] && iwpriv "$device" rxchainmask $rxchainmask

    # An extra IE is provided for Intel interop
    [ -f /proc/sys/dev/ath/htdupieenable ] &&
    echo 1 > /proc/sys/dev/ath/htdupieenable

    for vif in $vifs; do
        local vif_txpower= nosbeacon=
        config_get enc "$vif" encryption
        config_get mode "$vif" mode

        ifname=$(wlanconfig ath create wlandev "$device" wlanmode "$mode" ${nosbeacon:+nosbeacon})
        [ $? -ne 0 ] && {
            echo "enable_atheros($device): Failed to set up $mode vif $ifname" >&2
            continue
        }
        config_set "$vif" ifname "$ifname"

        [ -n "$extoffset" ] && iwpriv "$ifname" extoffset $extoffset
        iwpriv "$ifname" txqueuelen $txqlen
        iwpriv "$ifname" cwmmode $cwmmode

        config_get dbglvl "$vif" dbglvl 0
        iwpriv "$ifname" dbgLVL $dbglvl

        config_get_bool shortgi "$vif" shortgi 1
        iwpriv "$ifname" shortgi $shortgi

        config_get_bool bgscan "$vif" bgscan 0
        iwpriv "$ifname" bgscan $bgscan

        config_get ampdumin "$device" ampdumin 32768
        iwpriv "$ifname" ampdumin $ampdumin

        iwpriv "$ifname" mode "$chmode"

        config_get pureg "$vif" pureg 0
        iwpriv "$ifname" pureg "$pureg"

        config_get puren "$vif" puren 0
        iwpriv "$ifname" puren "$puren"

        iwconfig "$ifname" channel "$channel" >/dev/null 2>/dev/null

        config_get_bool hidden "$vif" hidden 0
        iwpriv "$ifname" hide_ssid "$hidden"

        config_get ssid "$vif" ssid
        [ -n "$ssid" ] && {
            iwconfig "$ifname" essid on
            iwconfig "$ifname" essid "$ssid"
        }

        set_wifi_up "$vif" "$ifname"

        config_get_bool wps "$vif" wps 0
        [ $wps -eq 1 ] && append hostapd_vifs "$vif" " "

        case "$enc" in
            WEP|wep)
                for idx in 1 2 3 4; do
                    config_get key "$vif" "key${idx}"
                    iwconfig "$ifname" enc "[$idx]" "${key:-off}"
                done
                config_get key "$vif" key
                key="${key:-1}"
                case "$key" in
                    [1234]) iwconfig "$ifname" enc "[$key]";;
                    *) iwconfig "$ifname" enc "$key";;
                esac
                ;;
            psk*|wpa*)
                # needn't to add vif again if wps has added it.
                [ $wps -eq 0 ] && append hostapd_vifs "$vif" " "
                config_get key "$vif" key
                ;;
        esac

        config_get nrates "$vif" nrates
        [ -n "$nrates" ] && iwpriv "$ifname" set11NRates "${nrates%%.*}"

        config_get nretries "$vif" nretries
        [ -n "$nretries" ] && iwpriv "$ifname" set11NRetries "${nretries%%.*}"

        config_get frag "$vif" frag
        [ -n "$frag" ] && iwconfig "$ifname" frag "${frag%%.*}"

        config_get rts "$vif" rts
        [ -n "$rts" ] && iwconfig "$ifname" rts "${rts%%.*}"

        config_get_bool wmm "$vif" wmm
        [ -n "$wmm" ] && iwpriv "$ifname" wmm "$wmm"

        config_get_bool doth "$vif" doth 0
        [ -n "$doth" ] && iwpriv "$ifname" doth "$doth"
        ifconfig "$ifname" up

        local net_cfg bridge
        net_cfg="$(find_net_config "$vif")"
        [ -z "$net_cfg" ] || {
            bridge="$(bridge_interface "$net_cfg")"
            config_set "$vif" bridge "$bridge"
            start_net "$ifname" "$net_cfg"
        }

        config_get ssid "$vif" ssid
        [ -n "$ssid" ] && {
            iwconfig "$ifname" essid on
            iwconfig "$ifname" essid "$ssid"
        }

        set_wifi_up "$vif" "$ifname"

        case "$mode" in
            ap)
                config_get_bool isolate "$vif" isolate 0
                iwpriv "$ifname" ap_bridge "$((isolate^1))"
                ;;
        esac
    done

    [ -n "$hostapd_vifs" ] && madwifi11n_hostapd_setup_vif $device "$hostapd_vifs"

    return 0
}

disable_madwifi11n() {
    [ -f /sbin/update-wifi -a -f /sbin/wlan ] && {
        /sbin/wlan down
        return
    }
    unload_madwifi11n_driver
    local device="$1"

    set_wifi_down "$device"

    include /lib/network
    for dev in `cat /proc/net/wireless | grep ath | sed -e 's/ //g' | awk -F: '{print $1}'`; do
        [ -f "/var/run/wifi-${dev}.pid" ] &&
        kill "$(cat "/var/run/wifi-${dev}.pid")"
        ifconfig "$dev" down
        unbridge "$dev"
        wlanconfig "$dev" destroy
    done
    for pid in `pidof hostapd`; do
       grep "hostapd-topology" /proc/$pid/cmdline >/dev/null && \
           kill $pid
    done
    rm -rf /tmp/hostapd-*.conf
    return 0
}

detect_madwifi11n() {
	devidx=0
	config_load wireless
	for dev in $(cat /proc/bus/pci/devices | grep 168c | awk '{print $1}'); do

                [ $devidx -eq 0 ] && {
		    mode_11n=""
		    mode_band="g"
                    channel=5
                } || {
		    mode_11n=""
		    mode_band="a"
                    channel=36
                }

		cat <<EOF
config wifi-device  wifi$devidx
	option type     madwifi11n
	option channel  ${channel}
	option hwmode	11${mode_11n}${mode_band}
	# REMOVE THIS LINE TO ENABLE WIFI:
	option disabled 1

config wifi-iface
	option device   wifi$devidx
	option network  lan
	option mode     ap
	option ssid     OpenWrt
	option encryption none

EOF
		devidx=$(($devidx + 1))
	done
}

