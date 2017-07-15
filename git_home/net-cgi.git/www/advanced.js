function close_all_sub(click_name)/* fold all menus, except the menu which user click */
{
	var sub_list = "setup,usb,security,green,admin,advanced";
	var sub_array = sub_list.split(',');
	for ( i=0; i< sub_array.length; i++)
	{
		var button_name = sub_array[i]+"_bt";
		var sub_name = sub_array[i]+"_sub";
		if( sub_name != click_name )
		{
			var div_name = top.document.getElementById(button_name);
			if( !div_name)
				continue;
			settingClass(div_name, "advanced_white_close_button");

			top.document.getElementById(sub_name).style.display="none";
		}
	}
}

function open_or_close_sub(name)
{
	/* to fix bug 23268, when user want to unfold one menus, fold the other menus. */
	var button_name= name+"_bt";
	var sub_name= name+"_sub";
	var open_flag= top.document.getElementById(sub_name).style.display;
	
	close_all_sub(sub_name);/* fold all menus first, except the menu which user click*/

	var button_div = top.document.getElementById(button_name);
	if( open_flag == "none")
	{
		settingClass(button_div, "advanced_white_open_button");
		top.document.getElementById(sub_name).style.display="";
		subItemsClass(sub_name);
		enabledItemsClass();
	}
	else
	{
		settingClass(button_div, "advanced_white_close_button");
		top.document.getElementById(sub_name).style.display="none";
	}
	change_menu_height();
	if ( name == "green" )
		alert("$greendl_popup_help1\n\n$greendl_popup_help2\n\n$greendl_popup_help3");
}

/* change the min-height of the fromframe if unfold "Advanced Setup" */
function change_menu_height()
{
	var footer_div = document.getElementById("footer");
	var is_double = (footer_div.className == "footer_double");
	var menu_height = document.getElementById("menu").clientHeight > 510 ? document.getElementById("menu").clientHeight : 510;
	var sUserAgent = navigator.userAgent;
	if(sUserAgent.indexOf("MSIE 9.0") > -1)
		document.getElementById("middle").style.minHeight = is_double ? (menu_height + 87)+"px" : (menu_height+ 45)+"px";
	else if(isIE_or_Opera() && IE_version() !=9 && IE_version() !=11)
	{
		var height = is_double ? document.documentElement.clientHeight - 190 : document.documentElement.clientHeight - 147;
		menu_height = height > menu_height ? height : menu_height;
		document.getElementById("container").style.height = is_double ? menu_height+93 : menu_height+ 50;
		document.getElementById("middle").style.height = is_double ?  menu_height+87 : menu_height+ 45;
		document.getElementById("formframe_div").style.height = menu_height;
	}
	else
		document.getElementById("middle").style.minHeight = is_double ? (menu_height + 87)+"px" : (menu_height+ 45)+"px";
}

function change_size()
{
	setFooterClass();
	var footer_div = document.getElementById("footer");
	var is_double = (footer_div.className == "footer_double");

	var sUserAgent = navigator.userAgent;
	if(sUserAgent.indexOf("MSIE 9.0") > -1)
	{
		var menu_words = document.getElementById("menu");
		menu_words.className = "menu_ie9_words";
	}
	
	if(isIE_or_Opera() && IE_version() !=9 && IE_version() !=11)
	{
		/* to calculate the width */
		var width = document.documentElement.clientWidth - 40;
		document.getElementById("top").style.width = width > 820 ? width : "820px" ;
		document.getElementById("container").style.width = width > 820 ? width : "820px" ;
		document.getElementById("formframe_div").style.width = width > 820 ? width - 195 : "625px";
	}
	document.getElementById("formframe_div").style.bottom = is_double ? "88px" : "45px";

	change_menu_height();
}

function settingClass(obj, class_name)
{
	var height = obj.getElementsByTagName("span")[0].clientHeight;
	if(height > 30)
		obj.className = class_name + "_triple";
	else if(height > 15)
		obj.className = class_name + "_double";
	else
		obj.className = class_name;
}

function subItemsClass(argv)
{
	var sub_menu;
	var items;
	var words_height;
	var i, num;

	for(num=0; num<arguments.length; num++)
	{
		sub_menu = top.document.getElementById(arguments[num]);
		if(sub_menu.style.display == "none") continue;
		items = sub_menu.getElementsByTagName("dt");

		for(i=0; i<items.length; i++)
		{
			if( top.old_div == items[i] ) break;
			words_height = items[i].getElementsByTagName("a")[0].clientHeight;
			if(words_height > 28 )
				items[i].className = "long_name";
			else if( words_height > 14 )
				items[i].className = "middle_name";
			else
				items[i].className = "sub_back";
		}
	}
}

function clickSubMenu(current_div, change_id)
{
	if( top.old_div != "")
	{
		var old_div_class = top.old_div.className;
		if( old_div_class == "sub_back_purple" || old_div_class == "sub_back_purple_usb")
			top.old_div.className = "sub_back";
		else if ( old_div_class == "sub_back_purple_double" || old_div_class == "sub_back_purple_usb_double")
			top.old_div.className = "middle_name";
		else if ( old_div_class == "sub_back_purple_triple")
			top.old_div.className = "long_name";
	}
	if( current_div == "0" )
	{
		top.old_div = "";
		return;
	}
	var current_class = current_div.className;
	if( current_class == "sub_back" )
	{
		if( change_id == "usb_basic" || change_id == "usb_adv" || change_id == "media" || change_id == "userset")
			current_div.className = "sub_back_purple_usb";
		else
			current_div.className = "sub_back_purple";
	}
	else if( current_class == "middle_name")
	{
		if( change_id == "usb_basic" || change_id == "usb_adv" || change_id == "media" || change_id == "userset")
			current_div.className = "sub_back_purple_usb_double";
		else
			current_div.className = "sub_back_purple_double";
	}
	else if( current_class == "long_name")
		current_div.className = "sub_back_purple_triple";
	top.old_div = current_div;
}
var array_name = ["wds_items", "ap_items", "wds_ap_items", "bridge_items"];
var enable_flags = ["enabled_wds", "enable_ap_flag", "enable_bridge_flag"];
var wds_items = [ "guest", "email", "schedule", "log"]; //split up the above 2 items into three items,because wds_items is controled by top.enabled_wds,  ap_items is controled by top.enable_ap_flag. wds_ap_items is controled by both of them. This array should not include "qos", as in QOS page "Enable WMM (Wi-Fi multimedia) settings" still can be used, and according to Spec v10 P135: When a feature is grey out also means the functionalities are disabled. so "qos" can not grey out.
var ap_items = ["lan", "ipv6", "parental", "wl_bridge"]; // "wds",for bug 30286 moved here
var wds_ap_items = ["internet", "wan", "block_site", "block_services", "forwarding_triggering", "dns", "static", "remote", "upnp", "traffic"];
var bridge_items = [ "internet", "wireless", "guest","wan", "lan","qos", "parental","access_control","vpn","block_site", "block_services", "email", "schedule","wds","wlap","forwarding_triggering", "dns", "static", "remote", "upnp", "traffic", "ipv6"];
function enabledItemsClass()
{
	var i, j;
	var  enable_flag_1, enable_flag_2, enable_flag_3;

	enable_flag_1 =  eval("top." + enable_flags[0]);
	enable_flag_2 =  eval("top." + enable_flags[1]);

	enable_flag_3 =  eval("top." + enable_flags[2]);
	for(i=0; i<array_name.length; i++)
	{
		var item_group;

		item_group = eval(array_name[i]);
		for(j=0; j<item_group.length; j++)
		{
			var cur_div = top.document.getElementById(item_group[j]);
			if( !cur_div)// if not find cur_div, continue
				continue;
			var height  = cur_div.getElementsByTagName("a")[0].clientHeight;
			if( cur_div.style.display == "none" ) break;

			if( ((i == 0 || i == 2) && enable_flag_1 == 1) 
				|| ((i == 1 || i == 2) && enable_flag_2 == 1))
			{
				if( height > 28 )
					cur_div.className = "long_grey";
				else if( height > 14 )
					cur_div.className = "middle_grey"
				else
					cur_div.className = "sub_grey";
			}
			else if (i == 3 && enable_flag_3 ==1)
			{
				if( height > 28 )
					cur_div.className = "long_grey";
				else if( height > 14 )
					cur_div.className = "middle_grey"
				else
					cur_div.className = "sub_grey";
			}
		}
	}
	if(top.have_lte_flag ==1 )
	{
		var cur_div = top.document.getElementById("mobile_internet");
		var cur_div_1= top.document.getElementById("internet");
		var height = cur_div.getElementsByTagName("a")[0].clientHeight;
		var height_1 = cur_div_1.getElementsByTagName("a")[0].clientHeight;
		if(top.broadband_wan_type == "AutoDetc")
		{
			if( height > 28 )
				cur_div.className = "long_grey";
			else if( height > 14 )
				cur_div.className = "middle_grey"
			else
				cur_div.className = "sub_grey";
		}
		else
		{
			if( height > 28 )
				cur_div.className = "long_name";
			else if( height > 14 )
				cur_div.className = "middle_name";
			else
				cur_div.className = "sub_back";
		}
		if(top.broadband_wan_type == "MyDetc")
		{
			if( height_1 > 28 )
				cur_div_1.className = "long_grey";
			else if( height_1 > 14 )
				cur_div_1.className = "middle_grey";
			else
				cur_div_1.className = "sub_grey";
		}
		else
		{
			if( height_1 > 28 )
				cur_div_1.className = "long_name";
			else if( height_1 > 14 )
				cur_div_1.className = "middle_name";
			else
				cur_div_1.className = "sub_back";
		}
	}
}

function menu_class_default()
{
	var menu_div;
	var content_length;

	menu_div = top.document.getElementById("home");
	settingClass(menu_div, "advanced_black_button");

	if(top.have_lte_flag !=1)
	{
		menu_div = top.document.getElementById("setup_wizard");
	if( top.enabled_wds == 1 || top.enable_ap_flag == 1 || top.enable_bridge_flag == 1)
			settingClass(menu_div, "advanced_grey_button");
		else
			settingClass(menu_div, "advanced_black_button");
	}

	menu_div = top.document.getElementById("wps");
	if( top.enabled_wps == 1 && top.enabled_wds == 0 && top.enable_bridge_flag == 0)
		settingClass(menu_div, "advanced_black_button");
	else
		settingClass(menu_div, "advanced_grey_button");
	
        var extensible_items = ["setup_bt", "usb_bt", "security_bt", "green_bt", "admin_bt", "advanced_bt"];
	var i;
	for(i=0; i<extensible_items.length; i++)
	{
		menu_div = top.document.getElementById(extensible_items[i]);
		if( !menu_div )
			continue;
		settingClass(menu_div, "advanced_white_close_button");
	}

	enabledItemsClass();
}

function menu_color_change( change_id )
{
	menu_class_default();

	var current_div = top.document.getElementById(change_id);
	
	if( change_id == "home" ||  change_id == "setup_wizard" || change_id == "wps" )
	{
		settingClass(current_div, "advanced_purple_button");
		clickSubMenu(0, change_id);
	}
	else
	{
		var parent_id = top.document.getElementById(change_id).parentNode.parentNode.id;
                var btn_id = parent_id.replace('sub', 'bt');
                var btn_div = top.document.getElementById(btn_id);
		settingClass(btn_div, "advanced_white_open_button");

		clickSubMenu(current_div, change_id);
	}
}

function click_adv_action(id)
{
        if( enable_action == 1 )
        {
		if( top.formframe.location.href.indexOf("LAN_lan.htm")!= -1 && id != "lan" && top.resev_table_same ==0)
		{
			var cf = top.formframe.document.forms[0];
			if(confirm("Do you want to apply the configuration?")== false)
			{
				cf.submit_flag.value="reset_resev";
				cf.submit();
			}
			else
			{
				if(top.formframe.checklan(cf))
					cf.submit();
			}
			return false;
		}
		if( id == "home" )
                {
			menu_color_change('home');
			if(enable_bridge_flag == "1")
				goto_formframe('RST_bridge_status.htm');
			else
				goto_formframe('RST_status.htm');
                }
                else if( id == "setup_wizard" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			menu_color_change('setup_wizard');
			if (((top.netgear_region.toUpperCase() == "WW" || top.netgear_region == "") && top.gui_region == "Russian" ) || top.netgear_region.toUpperCase() == "RU")
				goto_formframe('RU_WIZ_sel.htm');
			else
				goto_formframe('WIZ_sel.htm');
                }
                else if( id == "wps" && top.enabled_wps == 1 && top.enabled_wds == 0 && top.enable_bridge_flag != 1)
                {
			goto_formframe('Add_WPS_Client.htm');
			menu_color_change('wps');
                }
                else if( id == "internet" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			if(!(top.have_lte_flag==1 && top.broadband_wan_type == "MyDetc"))
			{
				goto_formframe("BAS_basic.htm");
				menu_color_change('internet');
			}
                }
                else if( id == "wireless" && top.enable_bridge_flag != 1)
                {
			goto_formframe("WLG_wireless.htm");
			menu_color_change('wireless');
                }
                else if( id == "guest" && top.enabled_wds == 0 && top.enable_bridge_flag != 1)
                {
			goto_formframe("WLG_wireless_guest1.htm");
			menu_color_change('guest');
                }
                /*else if( id == "guest_a" && top.enabled_wds == 0 )
                {
                        goto_formframe("WLG_wireless_guestA1.htm");
                        menu_color_change('guest_a');
                }*/
                else if( id == "wan" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			goto_formframe("WAN_wan.htm");
			menu_color_change('wan');
                }
                else if( id == "lan" && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			goto_formframe("LAN_lan.htm");
			menu_color_change('lan');
                }
                else if( id == "qos" && top.enable_bridge_flag != 1)
                {
			goto_formframe("QOS_wait.htm");
			menu_color_change('qos');
                }
                else if( id == "usb_basic" )
                {
			goto_formframe("USB_basic.htm");
			menu_color_change('usb_basic');
                }
                else if( id == "usb_adv" )
                {
			goto_formframe("USB_advanced.htm");
			menu_color_change('usb_adv');
                }
		else if( id == "userset" )
                {
                       goto_formframe("hdd_usr_setup.htm");
                       menu_color_change('userset');
                }
                else if( id == "media" )
                {
			goto_formframe("UPNP_media.htm");
			menu_color_change('media');
                }
		else if( id == "cloud" )
		{
			goto_formframe("/cgi-bin/RMT_invite.htm");
			menu_color_change('cloud');
		}
		else if( id == "print" )
		{
			goto_formframe("RMT_print.htm");
			menu_color_change('print');
		}
		else if( id == "vault" )
		{
			goto_formframe("RMT_vault.htm");
			menu_color_change('vault');
		}
		else if( id == "mobile" )
		{
			goto_formframe("BAS_mobile.htm");
			menu_color_change('mobile');
		}
		else if( id == "parental" && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			open_window('parental_ctrl.htm');
			menu_color_change('parental');
                }
                else if( id == "block_site" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			goto_formframe("BKS_keyword.htm");
			menu_color_change('block_site');
                }
                else if( id == "block_services" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1) 
                {
			goto_formframe("BKS_service.htm");
			menu_color_change('block_services');
                }
                else if( id == "schedule" && top.enabled_wds == 0 && top.enable_bridge_flag != 1)//&& top.enable_ap_flag != 1
                {
			goto_formframe("FW_schedule.htm");
			menu_color_change('schedule');
                }
                else if( id == "email" && top.enabled_wds == 0 && top.enable_bridge_flag != 1)
                {
			goto_formframe("FW_email.htm");
			menu_color_change('email');
                }
		else if( id == "ipmac")
		{
			goto_formframe("ipmac.htm");
			menu_color_change('ipmac');
		}
                else if( id == "status" )
                {
			if(top.have_lte_flag ==1)
				goto_formframe("get_mobile_status.html");
			else
			{
				if(enable_bridge_flag == "1")
					goto_formframe('RST_bridge_status.htm');
				else
					goto_formframe("RST_status.htm");
			}
			menu_color_change('status');
                }
                else if( id == "log" && top.enabled_wds == 0 )
                {
			goto_formframe("FW_log.htm");
			menu_color_change('log');
                }
                else if( id == "attached" )
                {
			goto_formframe("DEV_device.htm");
			menu_color_change('attached');
                }
                else if( id == "bak_set" )
                {
			goto_formframe("BAK_backup.htm");
			menu_color_change('bak_set');
                }
                else if( id == "passwd" )
                {
			goto_formframe("PWD_password.htm");
			menu_color_change('passwd');
                }
                else if( id == "upgrade" )
                {
			goto_formframe("UPG_upgrade.htm");
			menu_color_change('upgrade');
                }
                else if( id == "plc" )
                {
			goto_formframe("PLC_wait_scan.htm");
			menu_color_change('plc');
                }
                else if( id == "wladv" )
                {
			goto_formframe("WLG_advanced.htm");
			menu_color_change('wladv');
                }
		else if( id == "wlap" && top.enable_bridge_flag != 1)
                {
                        goto_formframe("WLG_wap.htm");
                        menu_color_change('wlap');
                }
		else if( id == "wl_bridge" && top.enable_ap_flag != 1 )
                {
                        goto_formframe("WLG_bridge.htm");
                        menu_color_change('wl_bridge');
                }
                else if( id == "wds" && top.enable_bridge_flag != 1)//&& top.enable_ap_flag != 1 for bug 30286 moved here
                {
			goto_formframe("WLG_wds.htm");
			menu_color_change('wds');
                }
                else if( id == "forwarding_triggering" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			goto_formframe("FW_forward.htm");
			menu_color_change('forwarding_triggering');
                }
                else if( id == "dns" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
                {
			goto_formframe("DNS_ddns.htm");
			menu_color_change('dns');
                }
		else if( id == "static" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
		{
			goto_formframe("STR_routes.htm");
			menu_color_change('static');
		}
		else if( id == "remote" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
		{
			goto_formframe("FW_remote.htm");
			menu_color_change('remote');
		}
		else if( id == "usb" )
		{
			goto_formframe("USB_settings.htm");
			menu_color_change('usb');
		}
		else if( id == "upnp" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
		{
			goto_formframe("UPNP_upnp.htm");
			menu_color_change('upnp');
		}
		else if( id == "ipv6" && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
		{
			goto_formframe("IPv6_index.htm");
			menu_color_change('ipv6');
		}
		else if( id == "tr069" )
		{
			goto_formframe("tr069.htm");
			menu_color_change('tr069');
		}
		else if( id == "traffic" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
		{
			goto_formframe("traffic.htm");
			menu_color_change('traffic');
		}
		else if( id == "green_basic")
		{
			goto_formframe("green_download.htm");
			menu_color_change(id);
		}
		else if( id == "green_manager")
		{
			if( download_type == "http" )
				goto_formframe("GREEN_http_basic.htm");
			else if( download_type == "emule" )
				goto_formframe("GREEN_emule_basic.htm");
			else
				goto_formframe("GREEN_bt_basic.htm");
			menu_color_change(id);
		}
		else if( id == "lte_broadband" && top.have_lte_flag==1)
		{
			goto_formframe("BAS_lte_broadband.htm");
			menu_color_change(id);
		}
		else if( id == "mobile_internet" && top.broadband_wan_type != "AutoDetc")
		{
			top.click_lte_sim=0;
			goto_formframe("check_mobile_internet_wait.htm");
			menu_color_change(id);
		}
		else if( id == "sim" )
		{
			top.click_lte_sim=1;
			goto_formframe("check_sim_wait.htm");
			menu_color_change(id);
		}
		else if( id == "mobile_upg" )
		{
			goto_formframe("UPG_module.htm");
			menu_color_change(id);
		}
		else if( id == "access_control" && top.enable_bridge_flag != 1)
		{
			goto_formframe("access_control.htm");
			menu_color_change('access_control');
		}
		else if( id == "led" )
		{
			goto_formframe("LED_control_setting.htm");
			menu_color_change('led');
		}
		else if( id == "vpn" && top.enabled_wds == 0 && top.enable_ap_flag != 1 && top.enable_bridge_flag != 1)
		{
			goto_formframe("vpn_service.htm");
			menu_color_change('vpn');
		}
		else if( id == "fbwifi")
		{
			goto_formframe("fbwifi.htm");
			menu_color_change('fbwifi');
		}
		else if( id == "vlan" )
		{
			goto_formframe("VLAN_IPTV.htm");
			menu_color_change('vlan');
		}							                 
        }
	change_menu_height();
}

