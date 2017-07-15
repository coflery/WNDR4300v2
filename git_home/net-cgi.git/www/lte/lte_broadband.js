function click_ping()
{
	var cf=document.forms[0];
	if(cf.detect_method_rd[0].checked)
	{
		setDisabled(true, cf.hostname_tx);
		setDisabled(true, cf.myip_1,cf.myip_2,cf.myip_3,cf.myip_4);
	}
	else if(cf.detect_method_rd[1].checked)
	{
		setDisabled(false, cf.hostname_tx);
		setDisabled(true, cf.myip_1,cf.myip_2,cf.myip_3,cf.myip_4);
	}
	else if(cf.detect_method_rd[2].checked)
	{
		setDisabled(true, cf.hostname_tx);
		setDisabled(false, cf.myip_1,cf.myip_2,cf.myip_3,cf.myip_4);
	}
}

function change_mode()
{
	var cf=document.forms[0];
	if(cf.conn_mode.value!="0")
	{
		setDisabled(true, cf.detect_method_rd[0],cf.detect_method_rd[1],cf.detect_method_rd[2]);
		setDisabled(true, cf.hostname_tx);
		setDisabled(true, cf.myip_1,cf.myip_2,cf.myip_3,cf.myip_4);
		setDisabled(true, cf.retry_inter_tx,cf.failover_tx,cf.failover_resume_tx);
		setDisabled(true, cf.enable_hardware_ch,cf.failover_sec_tx);

	}
	else
	{
		setDisabled(false, cf.detect_method_rd[0],cf.detect_method_rd[1],cf.detect_method_rd[2]);
		setDisabled(false, cf.retry_inter_tx,cf.failover_tx,cf.failover_resume_tx);
		setDisabled(false, cf.enable_hardware_ch,cf.failover_sec_tx);
		click_ping();

	}

}

function check_lte_broadband()
{
	cf = document.forms[0];

	if(cf.conn_mode.value == "0" )
		cf.hidden_wan_type.value ="failover";
	else if(cf.conn_mode.value == "1" )
		cf.hidden_wan_type.value ="MyDetc";
	else if(cf.conn_mode.value == "2" )
		cf.hidden_wan_type.value ="AutoDetc";

	if(old_wan_type == "dhcp")
		cf.primary_link_hid.value = "dhcp";
	else if(old_wan_type == "pppoe")
		cf.primary_link_hid.value = "pppoe";
	else if(old_wan_type == "pptp")
		cf.primary_link_hid.value = "pptp";
	else if(old_wan_type == "l2tp")
		cf.primary_link_hid.value = "l2tp";
	else if(old_wan_type == "static")
		cf.primary_link_hid.value = "static";

	if( cf.detect_method_rd[0].checked == true )
		cf.hidden_detect_method.value = "0";
	else if( cf.detect_method_rd[1].checked == true )
		cf.hidden_detect_method.value = "1";
	else
		cf.hidden_detect_method.value = "2";

	cf.hidden_detect_ip.value = cf.myip_1.value+'.'+cf.myip_2.value+'.'+cf.myip_3.value+'.'+cf.myip_4.value;
	if( cf.detect_method_rd[1].checked == true )
	{
		if( cf.hostname_tx.value == "" )
		{
			alert("Hostname should not be NULL");
			return false;
		}
	}

	if( cf.detect_method_rd[2].checked == true )
	{
		if( cf.hidden_detect_ip.value != "..." )
		{
			if(checkipaddr(cf.hidden_detect_ip.value)==false)
			{
				alert("$invalid_myip");
				return false;
			}
		}
		else
		{
			alert("ip miss");
			return false;
		}
	}

	if( cf.retry_inter_tx.value == "" || cf.failover_tx.value == "" || cf.failover_resume_tx.value == "" )
	{
		alert("parameter miss");
		return false;
	}

	if( cf.enable_hardware_ch.checked == true )
	{
		cf.hidden_enable_hardware.value = "1";
		if(cf.failover_sec_tx.value == "")
		{
			alert("parameter miss");
			return false;
		}

	}
	else
		cf.hidden_enable_hardware.value = "0";

	if(cf.hidden_wan_type.value == "MyDetc")
	{
		if(lte_vendor == "sierra")
			wanproto_type="3g";
		else
			wanproto_type="lte";
	}
	else if(cf.hidden_wan_type.value == "AutoDetc")
	{
		if(internet_type=="1")
			wanproto_type="dhcp";
		else
		{
			if(internet_ppp_type =="0")
				wanproto_type="pppoe";
			else if(internet_ppp_type =="1")
				wanproto_type="pptp";
			else if(internet_ppp_type =="4")
				wanproto_type="l2tp";
			else
				wanproto_type="pppoe";
		}
	}

	if(cf.hidden_wan_type.value != broadband_wan_type)
		cf.new_wan_type.value=wanproto_type;
	mtu_change(wanproto_type);

	return true;
}
