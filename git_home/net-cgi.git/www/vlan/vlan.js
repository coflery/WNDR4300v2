function check_vlan_input(cf,flag)
{
	if(!(flag=='edit' && default_internet ==1))
	{
		if (cf.vlan_name.value.length>10 || cf.vlan_name.value.length==0)
		{
			alert("$vlan_error11");
			return false;
		}
		for(i=1;i<=array_num;i++)
		{
			var str=eval( 'vlanArray' + i ).replace(/&#92;/g, "\\").replace(/&lt;/g,"<").replace(/&gt;/g,">").replace(/&#40;/g,"(").replace(/&#41;/g,")").replace(/&#34;/g,'\"').replace(/&#39;/g,"'").replace(/&#35;/g,"#").replace(/&#38;/g,"&");
			var str_info=str.split(' ');
			if(str_info[1] == cf.vlan_name.value && (!(flag == 'edit' && sel_num==i )))
			{
				alert("$vlan_error4_1 " +cf.vlan_name.value +" $vlan_error4_2");
				return false;
			}
		}
	}
	if(cf.vlan_id.value.length == 0)
	{
		alert("$vlan_error1");
		return false;
	}
	var str_tmp=parseInt(cf.vlan_id.value,10);
	if(str_tmp <1 || str_tmp >4094)
	{
		alert("$vlan_error3");
		return false;
	}
	if(cf.vlan_priority.value=="")
		cf.vlan_priority.value="0";
	str_tmp=parseInt(cf.vlan_priority.value,10);
	if(str_tmp >7)
	{
		alert("$vlan_error2");
		return false;
	}
	if(!(flag=='edit' && default_internet ==1))
	{
		var wired=0;
		var wireless=0;
		if(cf.vlan_port4.checked==true)
			wired += 8;
		if(cf.vlan_port3.checked==true)
			wired += 4;
		if(cf.vlan_port2.checked==true)
			wired += 2;
		if(cf.vlan_port1.checked==true)
			wired += 1;
		if(cf.vlan_bgn_wlan.checked==true)
			wireless += 1;
		if(cf.vlan_bgn_guest.checked==true)
			wireless += 4;
		if(cf.vlan_an_wlan.checked==true)
			wireless += 2;
		if(cf.vlan_an_guest.checked==true)
			wireless += 8;
		if(wired==15 && wireless==15)
		{
			alert("$vlan_error6");
			return false;
		}
		if(wired==0 && wireless==0)
		{
			alert("$vlan_error5");
			return false;
		}
		cf.hid_wired_port.value=wired;
		cf.hid_wireless_port.value=wireless;
	}
	
	if(flag=='edit')
	{
		if(default_internet ==1)
			cf.hid_vlan_name.value=each_info[1];
		else
			cf.hid_vlan_name.value=cf.vlan_name.value;
	}

	return true;
}

function click_add_btn(cf)
{
	if(array_num>=10)
	{
		alert("$vlan_error9");
		return false;
	}
	else
	{
		cf.hid_enable_vlan.value="1";
		cf.hid_vlan_type.value="1";
		cf.action="/apply.cgi?/VLAN_add.htm timestamp="+ts;
	}
	cf.submit_flag.value="vlan_add";
	return true;
}

function check_iptv_input(cf)
{
	var wired=0;
	var wireless=0;
	if(cf.vlan_port4.checked==true)
	{
		wired += 8;
		cf.hid_bri_lan4.value="1";
	}
	else
		cf.hid_bri_lan4.value="0";
	if(cf.vlan_port3.checked==true)
	{
		wired += 4;
		cf.hid_bri_lan3.value="1";
	}
	else
		cf.hid_bri_lan3.value="0";
	if(cf.vlan_port2.checked==true)
	{
		wired += 2;
		cf.hid_bri_lan2.value="1";
	}
	else
		cf.hid_bri_lan2.value="0";
	if(cf.vlan_port1.checked==true)
	{
		wired += 1;
		cf.hid_bri_lan1.value="1";
	}
	else
		cf.hid_bri_lan1.value="0";
	if(cf.vlan_bgn_wlan.checked==true)
	{
		wireless += 1;
		cf.hid_brig_ssid1.value="1";
	}
	else
		cf.hid_brig_ssid1.value="0";
	if(cf.vlan_bgn_guest.checked==true)
	{
		wireless += 4;
		cf.hid_brig_guest_ssid1.value="1";
	}
	else
		cf.hid_brig_guest_ssid1.value="0";
	if(cf.vlan_an_wlan.checked==true)
	{
		wireless += 2;
		cf.hid_brig_ssid2.value="1";
	}
	else
		cf.hid_brig_ssid2.value="0";
	if(cf.vlan_an_guest.checked==true)
	{
		wireless += 8;
		cf.hid_brig_guest_ssid2.value="1";
	}
	else
		cf.hid_brig_guest_ssid2.value="0";
	if(wired==15 && wireless==15)
	{
		alert("$vlan_error6");
		return false;
	}
	if(wired==0 && wireless==0)
	{
		alert("$vlan_error5");
		return false;
	}
	cf.hid_iptv_mask.value=wired;
	return true;
}

function click_edit_btn(cf)
{
	var select_num;
	var count_select=0;
	if (array_num == 0)
	{
		alert("$port_edit");
		return false;
	}
	if (array_num == 1)
	{
		if(cf.vlanSelect.checked == true)
		{
			count_select++;
			select_num=parseInt(cf.vlanSelect.value);
		}
	}
	else
	{
		for(i=0;i<array_num;i++)
		{
			if(cf.vlanSelect[i].checked == true)
			{
				count_select++;
				select_num=parseInt(cf.vlanSelect[i].value);
			}
		}
	}
	if(count_select==0)
	{
		alert("$port_edit");
		return false;
	}
	else
	{
		cf.hid_enable_vlan.value="1";
		cf.hid_vlan_type.value="1";
		cf.select_edit_num.value =select_num;
		cf.submit_flag.value="vlan_edit";
		cf.action="/apply.cgi?/VLAN_edit.htm timestamp="+ts;
	}
	return true;
}

function click_delete_btn(cf)
{
	var count_select=0;
	var select_num;
	if ( array_num == 0 )
	{
		alert("$port_del");
		return false;
	}
	if (array_num == 1)
	{
		if(cf.vlanSelect.checked == true)
		{
			count_select++;
			select_num=parseInt(cf.vlanSelect.value);
		}
	}
	else
	{
		for(i=0;i<array_num;i++)
		{
			if(cf.vlanSelect[i].checked == true)
			{
				count_select++;
				select_num=parseInt(cf.vlanSelect[i].value);
			}
		}
	}
	if(count_select==0)
	{
		alert("$port_del");
		return false;
	}
	else
	{
		var sel_str=eval( 'vlanArray' + select_num ).replace(/&#92;/g, "\\").replace(/&lt;/g,"<").replace(/&gt;/g,">").replace(/&#40;/g,"(").replace(/&#41;/g,")").replace(/&#34;/g,'\"').replace(/&#39;/g,"'").replace(/&#35;/g,"#").replace(/&#38;/g,"&");
		var sel_info=sel_str.split(' ');

		if(confirm("$vlan_warn1"+" "+ sel_info[1]+"?") ==false)
			return false;
		if(sel_info[1]=="Internet" || (sel_info[1]=="Intranet" && is_for_RU==1))
		{
			alert(sel_info[1]+" $vlan_port_del_msg");
			return false;
		}
		cf.hid_enable_vlan.value="1";
		cf.hid_vlan_type.value="1";
		cf.select_del_num.value =select_num;
		cf.submit_flag.value="vlan_delete";
	}

	return true;
}

function click_apply(cf)
{
	if(cf.vlan_enable.checked==true)
	{
		if(cf.vlan_iptv_type[1].checked==true)
		{					
			var count_enable=0;
			var sel_list="";
			var port1=port2=port3=port4=port5=port6=port7=port8=0;
			for(i=1;i<=array_num;i++)
			{
				var boxName= "vlan_check"+i;
				if(document.getElementById(boxName).checked == true)
				{
					var sel_str=eval( 'vlanArray' + i );
					var sel_info=sel_str.split(' ');
					var lan_port=parseInt(sel_info[4],10);
					var wlan_port=parseInt(sel_info[5],10);
					if(lan_port>=8 && lan_port<=15)
						port1++;
					if((lan_port>=12 && lan_port<=15) || (lan_port>=4 && lan_port<=7))
						port2++;
					if(lan_port==15 ||lan_port==14 ||lan_port==11 ||lan_port==10 ||lan_port==7 ||lan_port==6 ||lan_port==3 ||lan_port==2)
						port3++;
					if(lan_port%2 ==1)
						port4++;
					if(wlan_port>=8 && wlan_port<=15)
						port5++;
					if((wlan_port>=12 && wlan_port<=15) || (wlan_port>=4 && wlan_port<=7))
						port6++;
					if(wlan_port==15 ||wlan_port==14 ||wlan_port==11 ||wlan_port==10 ||wlan_port==7 ||wlan_port==6 ||wlan_port==3 ||wlan_port==2)
						port7++;
					if(wlan_port%2 ==1)
						port8++;
					sel_list+= i;
					sel_list+= "#";
					count_enable++;
				}
			}
			if(port1>1 ||port2>1 ||port3>1 ||port4>1 ||port5>1 ||port6>1 ||port7>1 ||port8>1)
			{
				alert("$vlan_port_dup");
				return false;
			}
			if(count_enable>6)
			{
				alert("$vlan_error10");
				return false;
			}
			else
			{
				cf.hid_enable_vlan.value="1";
				cf.hid_vlan_type.value="1";
				cf.hid_sel_list.value=sel_list;
				cf.hid_enabled_num.value=count_enable;
				cf.submit_flag.value="apply_vlan";
			}
		}
		else//bridge
		{
			check_iptv_input(cf);
			cf.hid_enable_vlan.value="1";
			cf.hid_vlan_type.value="0";
			cf.submit_flag.value="apply_iptv_edit";
		}
	}
	else
	{
		cf.hid_enable_vlan.value="0";
		cf.submit_flag.value="disable_vlan_iptv";
	}
	return true;
}
