#include "uhttpd-env.h"
#include "string.h"

struct env_config envs[] = {
#ifdef HOST_NAME
	{ "ENV_MODULE_ID", "cgi_module_id", HOST_NAME },
#else
	{ "ENV_MODULE_ID", "cgi_module_id", "WNDR4300" },
#endif
#ifdef CTL_MOD
	{ "ENV_CTL_MOD", "cgi_ctl_mod", CTL_MOD },
#else
	{ "ENV_CTL_MOD", "cgi_ctl_mod", "wndr4300" },
#endif
#ifdef UPG_MOD
	{ "ENV_UPG_MOD", "cgi_upg_mod", UPG_MOD },
#else
	{ "ENV_UPG_MOD", "cgi_upg_mod", "WNDR4300" },
#endif
#ifdef NET_IFNAME
	{ "ENV_NET_IFNAME", "cgi_net_ifname", NET_IFNAME },
#else
	{ "ENV_NET_IFNAME", "cgi_net_ifname", "eth1" },
#endif
#ifdef LAN_WIRED_IFNAME
	{ "ENV_LAN_IFNAME", "cgi_lan_ifname", LAN_WIRED_IFNAME },
#else
	{ "ENV_LAN_IFNAME", "cgi_lan_ifname", "eth0" },
#endif
#ifdef IPv6_LAN_IFNAME
	{ "ENV_LANv6_IFNAME", "cgi_ipv6_ifname", IPv6_LAN_IFNAME },
#else
	{ "ENV_LANv6_IFNAME", "cgi_ipv6_ifname", "br0" },
#endif
#ifdef SMARTAGENT_VERSION
	{ "ENV_SMARTAGENT_INFO", "cgi_smartagent_info", SMARTAGENT_VERSION },
#else
	{ "ENV_SMARTAGENT_INFO", "cgi_smartagent_info", "3.0" },
#endif
#ifdef FIREWALL_INFO
	{ "ENV_FIREWALL_INFO", "cgi_firewall_info", FIREWALL_INFO },
#else
	{ "ENV_FIREWALL_INFO", "cgi_firewall_info", "net-wall 2.0" },
#endif
#ifdef VPN_INFO
	{ "ENV_VPN_INFO", "cgi_vpn_info", VPN_INFO },
#else
	{ "ENV_VPN_INFO", "cgi_vpn_info", "ppp 2.4.3" },
#endif
#ifdef MAX_BANDWIDTH
	{ "ENV_MAX_BANDWIDTH", "cgi_max_bandwidth", MAX_BANDWIDTH },
#else
	{ "ENV_MAX_BANDWIDTH", "cgi_max_bandwidth", "1000" },
#endif
#ifdef IP_PATH
	{ "ENV_IP_PATH", "cgi_ip_path", IP_PATH },
#else
	{ "ENV_IP_PATH", "cgi_ip_path", "ip" },
#endif
#ifdef DETWAN_PATH
	{ "ENV_DETWAN_PATH", "cgi_detwan_path", DETWAN_PATH },
#else
	{ "ENV_DETWAN_PATH", "cgi_detwan_path", "0" },
#endif

#ifdef NTPST_POSTION
	{ "ENV_NTPST_POSTION", "cgi_ntpst_postion", NTPST_POSTION },
#else
	{ "ENV_NTPST_POSTION", "cgi_ntpst_postion", "2048" },
#endif
#ifdef STAMAC_POSTION
	{ "ENV_STAMAC_POSTION", "cgi_stamac_postion", STAMAC_POSTION },
#else
	{ "ENV_STAMAC_POSTION", "cgi_stamac_postion", "2052" },
#endif
#ifdef MAX_POST_SIZE
	{ "ENV_MAX_POST_SIZE", "cgi_max_post_size", MAX_POST_SIZE },
#else
	{ "ENV_MAX_POST_SIZE", "cgi_max_post_size", "26214400" },//25MB
#endif
#ifdef BUFSIZE
	{ "ENV_BUFSIZE", "cgi_bufsize", BUFSIZE },
#else
	{ "ENV_BUFSIZE", "cgi_bufsize", "2048" },
#endif
#ifdef PHY_MEMORY
	{ "ENV_PHY_MEMORY", "cgi_phy_memory", PHY_MEMORY },
#else
	{ "ENV_PHY_MEMORY", "cgi_phy_memory", "32" },
#endif
#ifdef PHY_FLASH
	{ "ENV_PHY_FLASH", "cgi_phy_flash", PHY_FLASH },
#else
	{ "ENV_PHY_FLASH", "cgi_phy_flash", "4" },
#endif
#if (defined(CD_LESS) && CD_LESS )
	{ "ENV_CD_LESS", "cgi_cd_less", "1" },
#else
	{ "ENV_CD_LESS", "cgi_cd_less", "0" },
#endif
#if (defined(CD_LESS_DOWNLOAD) && CD_LESS_DOWNLOAD )
	{ "ENV_CD_LESS_DOWNLOAD", "cgi_cd_less", "1" },
#else
	{ "ENV_CD_LESS_DOWNLOAD", "cgi_cd_less", "0" },
#endif	
#if (defined(TOP_CENTRIA) && TOP_CENTRIA)
	{ "ENV_TOP_CENTRIA", "cgi_top_centria", "1" },
#else
	{ "ENV_TOP_CENTRIA", "cgi_top_centria", "0" },
#endif
#if (defined(IP_MAC) && IP_MAC)
	{ "ENV_IP_MAC", "cgi_ip_mac", "1" },
#else
	{ "ENV_IP_MAC", "cgi_ip_mac", "0" },
#endif
#if (defined(HAVE_IPv6) && HAVE_IPv6)
	{ "ENV_IPv6", "cgi_ipv6", "1" },
#else
	{ "ENV_IPv6", "cgi_ipv6", "0" },
#endif
#if (defined(IPv6_DNS_MANUAL) && IPv6_DNS_MANUAL)
	{ "ENV_IPv6_DNS_MANUAL", "cgi_ipv6_dns_manual", "1" },
#else
	{ "ENV_IPv6_DNS_MANUAL", "cgi_ipv6_dns_manual", "0" },
#endif
#if (defined(HAVE_TRAFFIC_METER) && HAVE_TRAFFIC_METER)
	{ "ENV_TRAFFIC_METER", "cgi_traffic_meter", "1" },
#else
	{ "ENV_TRAFFIC_METER", "cgi_traffic_meter", "0" },
#endif
#if (defined(HAVE_BLOCK_SITES) && HAVE_BLOCK_SITES)
	{ "ENV_BLOCK_SITES", "cgi_block_sites", "1" },
#else
	{ "ENV_BLOCK_SITES", "cgi_block_sites", "0" },
#endif
#if (defined(HAVE_TR069) && HAVE_TR069)
	{ "ENV_TR069", "cgi_tr069", "1" },
#else
	{ "ENV_TR069", "cgi_tr069", "0" },
#endif
#if (defined(HAVE_PLC) && HAVE_PLC)
	{ "ENV_PLC", "cgi_plc", "1" },
#else
	{ "ENV_PLC", "cgi_plc", "0" },
#endif
#if (defined(VC_RELEASE) && VC_RELEASE)
	{ "ENV_VC_RELEASE", "cgi_vc_release", "1" },
#else
	{ "ENV_VC_RELEASE", "cgi_vc_release", "0" },
#endif
#ifdef OPEN_SOURCE_MODEL_ID
	{ "ENV_OPEN_SOURCE_ID", "cgi_open_source_id", OPEN_SOURCE_MODEL_ID },
#else
	{ "ENV_OPEN_SOURCE_ID", "cgi_open_source_id", "NETGEAR" },
#endif
#if (defined(FIRMWARE_CHECKING) && FIRMWARE_CHECKING)
	{ "ENV_WEEKLY_CHECKING", "cgi_weekly_checking", "1" },
#else
	{ "ENV_WEEKLY_CHECKING", "cgi_weekly_checking", "0" },
#endif
#if (defined(HAVE_PIVOT_ROOT) && HAVE_PIVOT_ROOT)
	{ "ENV_PIVOT_ROOT", "cgi_pivot_root", "1" },
#else
	{ "ENV_PIVOT_ROOT", "cgi_pivot_root", "0" },
#endif
#if (defined(DNI_PARENTAL_CTL) && DNI_PARENTAL_CTL)
	{ "ENV_DNI_PARENTAL_CTL", "cgi_dni_parental_ctl", "1" },
#else
	{ "ENV_DNI_PARENTAL_CTL", "cgi_dni_parental_ctl", "0" },
#endif
#if (defined(HNAP_ON) && HNAP_ON)
	{ "ENV_HNAP_ON", "cgi_hnap_on", "1" },
#else
	{ "ENV_HNAP_ON", "cgi_hnap_on", "0" },
#endif
#if (defined(HAVE_THANK_LOGIN) && HAVE_THANK_LOGIN)
	{ "ENV_THANK_LOGIN", "cgi_thank_login", "1" },
#else
	{ "ENV_THANK_LOGIN", "cgi_thank_login", "0" },
#endif
#if (defined(HAVE_LOGS_CHECKBOX) && HAVE_LOGS_CHECKBOX)
	{ "ENV_LOGS_CHECKBOX", "cgi_logs_checkbox", "1" },
#else
	{ "ENV_LOGS_CHECKBOX", "cgi_logs_checkbox", "0" },
#endif
#if (defined(HAVE_TIVO) && HAVE_TIVO)
	{ "ENV_TIVO", "cgi_tivo", "1" },
#else
	{ "ENV_TIVO", "cgi_tivo", "0" },
#endif
#if (defined(USE_MTD_UTIL) && USE_MTD_UTIL)
	{ "ENV_USE_MTD_UTIL", "cgi_use_mtd_util", "1" },
#else
	{ "ENV_USE_MTD_UTIL", "cgi_use_mtd_util", "0" },
#endif
#if (defined(HAVE_EMAIL_SSL) && HAVE_EMAIL_SSL)
	{ "ENV_EMAIL_SSL", "cgi_email_ssl", "1" },
#else
	{ "ENV_EMAIL_SSL", "cgi_email_ssl", "0" },
#endif
#if (defined(HAVE_SMTP_PORT) && HAVE_SMTP_PORT)
	{ "ENV_SMTP_PORT", "cgi_smtp_port", "1" },
#else
	{ "ENV_SMTP_PORT", "cgi_smtp_port", "0" },
#endif
#if (defined(FORWARD_RANGE) && FORWARD_RANGE)
	{ "ENV_FORWARD_RANGE", "cgi_forword_range", "1" },
#else
	{ "ENV_FORWARD_RANGE", "cgi_forword_range", "0" },
#endif
#if (defined(HAVE_ACCESS_CONTROL) && HAVE_ACCESS_CONTROL)
	{ "ENV_ACCESS_CTL", "cgi_access_ctl", "1" },
#else
	{ "ENV_ACCESS_CTL", "cgi_access_ctl", "0" },
#endif
#if (defined(SMART_NETWORK_SUPPORT) && SMART_NETWORK_SUPPORT)
	{ "ENV_SMART_NETWORK", "cgi_smart_network", "1" },
#else
	{ "ENV_SMART_NETWORK", "cgi_smart_network", "0" },
#endif
#if (defined(HAVE_BRIDGE_MODE) && HAVE_BRIDGE_MODE)
	{ "ENV_BRIDGE_MODE", "cgi_bridge_mode", "1" },
#else
	{ "ENV_BRIDGE_MODE", "cgi_bridge_mode", "0" },
#endif
#ifdef MODE_1
	{ "ENV_MODE_1", "cgi_mode_1", MODE_1 },
#else
	{ "ENV_MODE_1", "cgi_mode_1", "54" },
#endif
#ifdef MODE_2
	{ "ENV_MODE_2", "cgi_mode_2", MODE_2 },
#else
	{ "ENV_MODE_2", "cgi_mode_2", "130" },
#endif
#ifdef MODE_3
	{ "ENV_MODE_3", "cgi_mode_3", MODE_3 },
#else
	{ "ENV_MODE_3", "cgi_mode_3", "300" },
#endif
#ifdef MODE_4
	{ "ENV_MODE_4", "cgi_mode_4", MODE_4 },
#else
	{ "ENV_MODE_4", "cgi_mode_4", "216" },
#endif
#ifdef MODE_5
	{ "ENV_MODE_5", "cgi_mode_5", MODE_5 },
#else
	{ "ENV_MODE_5", "cgi_mode_5", "450" },
#endif
#if (defined(HAVE_WIRELESS_AN) && HAVE_WIRELESS_AN)
	{ "ENV_WIRELESS_AN", "cgi_wireless_an", "1" },
#else
	{ "ENV_WIRELESS_AN", "cgi_wireless_an", "0" },
#endif
#if (defined(HAVE_WIRELESS_AC) && HAVE_WIRELESS_AC)
	{ "ENV_WIRELESS_AC", "cgi_wireless_ac", "1" },
#else
	{ "ENV_WIRELESS_AC", "cgi_wireless_ac", "0" },
#endif
#if (defined(HAVE_GUEST_NETWORK) && HAVE_GUEST_NETWORK)
	{ "ENV_GUEST_NETWORK", "cgi_guest_network", "1" },
#else
	{ "ENV_GUEST_NETWORK", "cgi_guest_network", "0" },
#endif
#if (defined(HAVE_WDS) && HAVE_WDS)
	{ "ENV_WDS", "cgi_wds", "1" },
#else
	{ "ENV_WDS", "cgi_wds", "0" },
#endif
#if (defined(WDS_SUPPORT_WPA) && WDS_SUPPORT_WPA)
	{ "ENV_WDS_SUPPORT_WPA", "cgi_wds_support_wpa", "1" },
#else
	{ "ENV_WDS_SUPPORT_WPA", "cgi_wds_support_wpa", "0" },
#endif
#if (defined(HAVE_AP_MODE) && HAVE_AP_MODE)
	{ "ENV_AP_MODE", "cgi_ap_mode", "1" },
#else
	{ "ENV_AP_MODE", "cgi_ap_mode", "0" },
#endif
#if (defined(DFS_CHANNEL) && DFS_CHANNEL)
	{ "ENV_DFS_CHANNEL", "cgi_dfs_channel", "1" },
#else
	{ "ENV_DFS_CHANNEL", "cgi_dfs_channel", "0" },
#endif
#if (defined(DFS_CHANNEL2) && DFS_CHANNEL2)
	{ "ENV_DFS_CHANNEL2", "cgi_dfs_channel2", "1" },
#else
	{ "ENV_DFS_CHANNEL2", "cgi_dfs_channel2", "0" },
#endif
#if (defined(DFS_CHANNEL3) && DFS_CHANNEL3)
	{ "ENV_DFS_CHANNEL3", "cgi_dfs_channel3", "1" },
#else
	{ "ENV_DFS_CHANNEL3", "cgi_dfs_channel3", "0" },
#endif
#if (defined(HAVE_WIRELESS_SCHEDULE) && HAVE_WIRELESS_SCHEDULE)
	{ "ENV_WIFI_SCHEDULE", "cgi_wifi_schedule", "1" },
#else
	{ "ENV_WIFI_SCHEDULE", "cgi_wifi_schedule", "0" },
#endif
#if (defined(HAVE_WIFI_SCHEDULE_SPECIAL_CASE) && HAVE_WIFI_SCHEDULE_SPECIAL_CASE)
	{ "ENV_WIFI_SCH_NOT_CHECK", "cgi_wifi_sch_not_check", "1" },
#else
	{ "ENV_WIFI_SCH_NOT_CHECK", "cgi_wifi_sch_not_check", "0" },
#endif
#if (defined(HAVE_VIDEO_NETWOR) && HAVE_VIDEO_NETWOR)
	{ "ENV_VIDEO_NETWORK", "cgi_video_network", "1" },
#else
	{ "ENV_VIDEO_NETWORK", "cgi_video_network", "0" },
#endif
#if (defined(HAVE_TRANSMIT_POWER_CONTROL) && HAVE_TRANSMIT_POWER_CONTROL)
	{ "ENV_TRANSMIT_POWER_CTL", "cgi_transmit_power_ctl", "1" },
#else
	{ "ENV_TRANSMIT_POWER_CTL", "cgi_transmit_power_ctl", "0" },
#endif
#if (defined(TXCTL_63_33) && TXCTL_63_33)
	{ "ENV_TXCTL_63_33", "cgi_txctl_63_33", "1" },
#else
	{ "ENV_TXCTL_63_33", "cgi_txctl_63_33", "0" },
#endif
#if (defined(HAVE_AN_COEXIST) && HAVE_AN_COEXIST)
	{ "ENV_AN_COEXIST", "cgi_an_coexist", "1" },
#else
	{ "ENV_AN_COEXIST", "cgi_an_coexist", "0" },
#endif
#if (defined(ADV_COEXISTENCE) && ADV_COEXISTENCE)
	{ "ENV_ADV_COEXISTENCE", "cgi_adv_coexistence", "1" },
#else
	{ "ENV_ADV_COEXISTENCE", "cgi_adv_coexistence", "0" },
#endif
#if (defined(HAVE_PROTECT_PIN) && HAVE_PROTECT_PIN)
	{ "ENV_PROTECT_PIN", "cgi_protect_pin", "1" },
#else
	{ "ENV_PROTECT_PIN", "cgi_protect_pin", "0" },
#endif

#if (defined(HAVE_L2TP) && HAVE_L2TP)
	{ "ENV_L2TP", "cgi_l2tp", "1" },
#else
	{ "ENV_L2TP", "cgi_l2tp", "0" },
#endif
#if (defined(HAVE_BIGPOND) && HAVE_BIGPOND)
	{ "ENV_BIGPOND", "cgi_bigpond", "1" },
#else
	{ "ENV_BIGPOND", "cgi_bigpond", "0" },
#endif
#if (defined(HAVE_PPPOE_MAC) && HAVE_PPPOE_MAC)
	{ "ENV_PPPOE_MAC", "cgi_pppoe_mac", "1" },
#else
	{ "ENV_PPPOE_MAC", "cgi_pppoe_mac", "0" },
#endif
#if (defined(RUSSIAN_PPTP) && RUSSIAN_PPTP)
	{ "ENV_RUSSIAN_PPTP", "cgi_russian_pptp", "1" },
#else
	{ "ENV_RUSSIAN_PPTP", "cgi_russian_pptp", "0" },
#endif
#if (defined(RUSSIAN_PPPOE) && RUSSIAN_PPPOE)
	{ "ENV_RUSSIAN_PPPOE", "cgi_russian_pppoe", "1" },
#else
	{ "ENV_RUSSIAN_PPPOE", "cgi_russian_pppoe", "0" },
#endif
#if (defined(HAVE_THIRD_DNS) && HAVE_THIRD_DNS)
	{ "ENV_THIRD_DNS", "cgi_third_dns", "1" },
#else
	{ "ENV_THIRD_DNS", "cgi_third_dns", "0" },
#endif
#if (defined(PPPOE_INTRANET) && PPPOE_INTRANET)
	{ "ENV_PPPOE_INTRANET", "cgi_pppoe_intranet", "1" },
#else
	{ "ENV_PPPOE_INTRANET", "cgi_pppoe_intranet", "0" },
#endif
#if (defined(HAVE_AUTO_CONN_RESET) && HAVE_AUTO_CONN_RESET)
	{ "ENV_AUTO_CONN_RESET", "cgi_auto_conn_reset", "1" },
#else
	{ "ENV_AUTO_CONN_RESET", "cgi_auto_conn_reset", "0" },
#endif
#if (defined(HAVE_VPN) && HAVE_VPN)
	{ "ENV_VPN", "cgi_vpn", "1" },
#else
	{ "ENV_VPN", "cgi_vpn", "0" },
#endif
#if (defined(IPv6_6RD) && IPv6_6RD)
	{ "ENV_IPv6_6RD", "cgi_ipv6_6rd", "1" },
#else
	{ "ENV_IPv6_6RD", "cgi_ipv6_6rd", "0" },
#endif
#if (defined(HAVE_OOKLA_SPEEDTEST) && HAVE_OOKLA_SPEEDTEST)
	{ "ENV_OOKLA_SPEEDTEST", "cgi_ookla_speedtest", "1" },
#else
	{ "ENV_OOKLA_SPEEDTEST", "cgi_ookla_speedtest", "0" },
#endif
#if (defined(HAVE_IGMP) && HAVE_IGMP)
	{ "ENV_IGMP", "cgi_igmp", "1" },
#else
	{ "ENV_IGMP", "cgi_igmp", "1" },
#endif
#if (defined(HAVE_BRIDGE_IPTV) && HAVE_BRIDGE_IPTV)
	{ "ENV_BRIDGE_IPTV", "cgi_bridge_iptv", "1" },
#else
	{ "ENV_BRIDGE_IPTV", "cgi_bridge_iptv", "0" },
#endif
#if (defined(HAVE_USB_STORAGE) && HAVE_USB_STORAGE)
	{ "ENV_USB_STORAGE", "cgi_usb_storage", "1" },
#else
	{ "ENV_USB_STORAGE", "cgi_usb_storage", "0" },
#endif
#if (defined(READYSHARE_REMOTE) && READYSHARE_REMOTE)
	{ "ENV_READYSHARE_REMOTE", "cgi_readyshare_remote", "1" },
#else
	{ "ENV_READYSHARE_REMOTE", "cgi_readyshare_remote", "0" },
#endif
#if (defined(READYSHARE_PRINT) && READYSHARE_PRINT)
	{ "ENV_READYSHARE_PRINT", "cgi_readyshare_print", "1" },
#else
	{ "ENV_READYSHARE_PRINT", "cgi_readyshare_print", "0" },
#endif
#if (defined(HAVE_VAULT) && HAVE_VAULT)
	{ "ENV_VAULT", "cgi_vault", "1" },
#else
	{ "ENV_VAULT", "cgi_vault", "0" },
#endif
#if (defined(PARTITION_CAPACITY_VIA_DF) && PARTITION_CAPACITY_VIA_DF)
	{ "ENV_USB_SCAN_VIA_DF", "cgi_usb_scan_via_df", "1" },
#else
	{ "ENV_USB_SCAN_VIA_DF", "cgi_usb_scan_via_df", "0" },
#endif
#if (defined(HDD_MULTI_USER) && HDD_MULTI_USER)
	{ "ENV_HDD_MULTI_USER", "cgi_hdd_multi_user", "1" },
#else
	{ "ENV_HDD_MULTI_USER", "cgi_hdd_multi_user", "0" },
#endif
#if (defined(GREEN_DOWNLOAD) && GREEN_DOWNLOAD)
	{ "ENV_NETGEAR_DOWNLOAD", "cgi_netgear_download", "1" },
#else
	{ "ENV_NETGEAR_DOWNLOAD", "cgi_netgear_download", "0" },
#endif
#if (defined(WW_GREEN_DOWNLOAD) && WW_GREEN_DOWNLOAD)
	{ "ENV_WW_NETGEAR_DOWNLOAD", "cgi_ww_netgear_download", "1" },
#else
	{ "ENV_WW_NETGEAR_DOWNLOAD", "cgi_ww_netgear_download", "0" },
#endif
#if (defined(READYSHARE_MOBILE) && READYSHARE_MOBILE)
	{ "ENV_READYSHARE_MOBILE", "cgi_readyshare_mobile", "1" },
#else
	{ "ENV_READYSHARE_MOBILE", "cgi_readyshare_mobile", "0" },
#endif
#if (defined(HAVE_BROADBAND) && HAVE_BROADBAND)
	{ "ENV_BROADBAND", "cgi_broadband", "1" },
#else
	{ "ENV_BROADBAND", "cgi_broadband", "0" },
#endif
#if (defined(HAVE_LTE) && HAVE_LTE)
	{ "ENV_LTE", "cgi_lte", "1" },
#else
	{ "ENV_LTE", "cgi_lte", "0" },
#endif
#if (defined(HAVE_FAST_LANE) && HAVE_FAST_LANE)
	{ "ENV_FAST_LANE", "cgi_fast_lane", "1" },
#else
	{ "ENV_FAST_LANE", "cgi_fast_lane", "0" },
#endif
#if (defined(HAVE_QOS) && HAVE_QOS)
	{ "ENV_QOS", "cgi_qos", "1" },
#else
	{ "ENV_QOS", "cgi_qos", "0" },
#endif
#if (defined(WW_QUICK_QOS) && WW_QUICK_QOS)
	{ "ENV_WW_QUICK_QOS", "cgi_ww_quick_qos", "1" },
#else
	{ "ENV_WW_QUICK_QOS", "cgi_ww_quick_qos", "0" },
#endif
#if (defined(QOS_CMD_BING) && QOS_CMD_BING)
	{ "ENV_QOS_CMD_BING", "cgi_qos_cmd_bing", "1" },
#else
	{ "ENV_QOS_CMD_BING", "cgi_qos_cmd_bing", "0" },
#endif
#if (defined(QOS_SPEC_2_0) && QOS_SPEC_2_0)
	{ "ENV_QOS_SPEC_2_0", "cgi_qos_spec_2_0", "1" },
#else
	{ "ENV_QOS_SPEC_2_0", "cgi_qos_spec_2_0", "0" },
#endif
#if (defined(QOS_TRUSTED_IP) && QOS_TRUSTED_IP)
	{ "ENV_QOS_TRUSTED_IP", "cgi_qos_trusted_ip", "1" },
#else
	{ "ENV_QOS_TRUSTED_IP", "cgi_qos_trusted_ip", "0" },
#endif

#if (defined(DNS_ORAY) && DNS_ORAY)
	{ "ENV_DNS_ORAY", "cgi_dns_oray", "1" },
#else
	{ "ENV_DNS_ORAY", "cgi_dns_oray", "0" },
#endif
#if (defined(DNS_3322) && DNS_3322)
	{ "ENV_DNS_3322", "cgi_dns_3322", "1" },
#else
	{ "ENV_DNS_3322", "cgi_dns_3322", "0" },
#endif
#if (defined(DNS_NO_IP) && DNS_NO_IP)
	{ "ENV_DNS_NO_IP", "cgi_dns_no_ip", "1" },
#else
	{ "ENV_DNS_NO_IP", "cgi_dns_no_ip", "0" },
#endif
#if (defined(DNS_WILDCARDS) && DNS_WILDCARDS)
	{ "ENV_DNS_WILDCARDS", "cgi_dns_wildcards", "1" },
#else
	{ "ENV_DNS_WILDCARDS", "cgi_dns_wildcards", "0" },
#endif
#if (defined(NETGEAR_DDNS) && NETGEAR_DDNS)
	{ "ENV_NETGEAR_DDNS", "cgi_netgear_ddns", "1" },
#else
	{ "ENV_NETGEAR_DDNS", "cgi_netgear_ddns", "0" },
#endif
#if (defined(HAVE_MULTILANGUAGE) && HAVE_MULTILANGUAGE)
	{ "ENV_MULTI_LANGUAGE", "cgi_multi_language", "1" },
#if (defined(NEW_MULTIPLE_LANGUAGE_19) && NEW_MULTIPLE_LANGUAGE_19)
	{ "ENV_NEW_MULTI_LANGUAGE", "cgi_new_multi_language", "1" },
#else
	{ "ENV_NEW_MULTI_LANGUAGE", "cgi_new_multi_language", "0" },
#endif
#if (defined(HAVE_PR) && HAVE_PR)
	{ "ENV_LANG_PR", "cgi_lang_pr", "1" },
#else
	{ "ENV_LANG_PR", "cgi_lang_pr", "0" },
#endif
#if (defined(HAVE_TR_PR) && HAVE_TR_PR)
	{ "ENV_LANG_TR_PR", "cgi_lang_tr_pr", "1" },
#else
	{ "ENV_LANG_TR_PR", "cgi_lang_tr_pr", "0" },
#endif
#if (defined(HAVE_TR_PR) && HAVE_TR_PR)
	{ "ENV_LANG_JP", "cgi_lang_jp", "1" },
#else
	{ "ENV_LANG_JP", "cgi_lang_jp", "0" },
#endif
#if (defined(HAVE_KO) && HAVE_KO)
	{ "ENV_LANG_KO", "cgi_lang_ko", "1" },
#else
	{ "ENV_LANG_KO", "cgi_lang_ko", "0" },
#endif
#if (defined(DEFAULT_ENG) && DEFAULT_ENG)
	{ "ENV_DEFAULT_ENG", "cgi_default_eng", "1" },
#else
	{ "ENV_DEFAULT_ENG", "cgi_default_eng", "0" },
#endif
#if (defined(DEFAULT_GR) && DEFAULT_GR)
	{ "ENV_DEFAULT_GR", "cgi_default_gr", "1" },
#else
	{ "ENV_DEFAULT_GR", "cgi_default_gr", "0" },
#endif
#if (defined(DEFAULT_PR) && DEFAULT_PR)
	{ "ENV_DEFAULT_PR", "cgi_default_pr", "1" },
#else
	{ "ENV_DEFAULT_PR", "cgi_default_pr", "1" },
#endif
#if (defined(DEFAULT_RU) && DEFAULT_RU)
	{ "ENV_DEFAULT_RU", "cgi_default_ru", "1" },
#else
	{ "ENV_DEFAULT_RU", "cgi_default_ru", "0" },
#endif
#if (defined(DEFAULT_PT) && DEFAULT_PT)
	{ "ENV_DEFAULT_PT", "cgi_default_pt", "1" },
#else
	{ "ENV_DEFAULT_PT", "cgi_default_pt", "0" },
#endif
#if (defined(DEFAULT_KO) && DEFAULT_KO)
	{ "ENV_DEFAULT_KO", "cgi_default_ko", "1" },
#else
	{ "ENV_DEFAULT_KO", "cgi_default_ko", "0" },
#endif
#else
	{ "ENV_MULTI_LANGUAGE", "cgi_multi_language", "0" },
#endif
	{ "ENV_DEFAULT_REGION", "cgi_default_region", DEFAULT_REGION },
	{ "ENV_DEFAULT_LANGUAGE", "cgi_default_langauge", DEFAULT_LANGUAGE },

	{ NULL, NULL, NULL }
};

void init_product_envs(void)
{
	struct env_config *p;
	char *value;
	
	for ( p = &envs[0]; p->env_name != NULL; p++ )
	{
		if ( !p->conf_name )
			continue;
		value = config_get(p->conf_name);
		if ( strcmp(value, "") )
		{
			strcpy(p->value,value);
		}
	}
}

void set_product_envs(void)
{
	struct env_config *p;

	for ( p = &envs[0]; p->env_name != NULL; p++ )
	{
		setenv(p->env_name, p->value, 1);
	}
}
