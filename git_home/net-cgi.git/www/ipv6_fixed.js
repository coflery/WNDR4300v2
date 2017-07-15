function check_ipv6_fixed(cf)
{
	if(enable_vpn == "1")
	{
		alert("$no_enable_ipv6");
		return false;
	}
	var i;
	var ip_trans_array = new Array("","","","","");//save IP address, if the IP input value is blank, change it to 0
	cf.ipv6_wan_ip_fixed.value = "";
	cf.ipv6_wan_gw_fixed.value = "";
	cf.ipv6_primary_dns_fixed.value = "";
	cf.ipv6_second_dns_fixed.value = "";
	cf.ipv6_lan_ip_fixed.value = "";

	var check_pri_dns = 1;/* 1: Primary DNS is not empty;*/
	var check_sec_dns = 1;/* 1: Second DNS is not empty;*/
	var check_gate_way = 1; /* 1: gateway is not empty; for bug 26010*/
	var pri_dns = "";
	var sec_dns = "";
	var gate_way = ""; // for bug 26010
	for( i=0; i<cf.IPv6_wan.length; i++ )
	{
		pri_dns = pri_dns + cf.IPv6_Pri_DNS[i].value;
		sec_dns = sec_dns + cf.IPv6_Sec_DNS[i].value;
		gate_way = gate_way + cf.IPv6_gw[i].value;
	}
	if( pri_dns == "" )
	{
		check_pri_dns = 0;
	}
	if( sec_dns == "")
	{
		check_sec_dns = 0;
	}
	if( gate_way == "") 
	{
		check_gate_way = 0;
	}
	

	/****************************check the IP legality***************************************/	
	for( i=0; i<cf.IPv6_wan.length; i++ )
	{
		/*---------------------------WAN IP---------------------------------*/
		if(check_ipv6_IP_address(cf.IPv6_wan[i].value) == false)
		{
			alert("$invalid_wan_ipv6_hex");
                        return false;
		}

		if(cf.IPv6_wan[i].value != "")			//00xx  ------>   xx
			cf.IPv6_wan[i].value = parseInt(cf.IPv6_wan[i].value, 16).toString(16);

		if(cf.IPv6_wan[i].value == "")
		{
			ip_trans_array[0] = ip_trans_array[0] + "0" + ":";
			cf.ipv6_wan_ip_fixed.value = cf.ipv6_wan_ip_fixed.value + "0" + ":";/* to fix bug 24430 */
		}
		else
		{
			ip_trans_array[0] = ip_trans_array[0] + cf.IPv6_wan[i].value + ":"; 
			cf.ipv6_wan_ip_fixed.value = cf.ipv6_wan_ip_fixed.value + cf.IPv6_wan[i].value + ":";
		}


		/*----------------------------Gateway-------------------------------------*/
		if( check_gate_way ){
		if(check_ipv6_IP_address(cf.IPv6_gw[i].value) == false)
		{
			alert("$invalid_wan_ipv6_gateway");
			return false;
		}

		if(cf.IPv6_gw[i].value != "")
			cf.IPv6_gw[i].value = parseInt(cf.IPv6_gw[i].value, 16).toString(16);

		if(cf.IPv6_gw[i].value == "")
		{
			ip_trans_array[1] = ip_trans_array[1] + "0" + ":";
			cf.ipv6_wan_gw_fixed.value = cf.ipv6_wan_gw_fixed.value +"0" + ":";/* to fix bug 24430 */
		}
		else
		{
			ip_trans_array[1] = ip_trans_array[1] + cf.IPv6_gw[i].value + ":";
			cf.ipv6_wan_gw_fixed.value = cf.ipv6_wan_gw_fixed.value + cf.IPv6_gw[i].value + ":";
		}
     }
 
		/*----------------------------Primary DNS------------------------------------*/	
		if( check_pri_dns ){// to fix bug 26923
			if(check_ipv6_DNS_address(cf.IPv6_Pri_DNS[i].value) == false)
			{
				alert("$invalid_ipv6_primary_dns_hex");
				return false;
			}

			if(cf.IPv6_Pri_DNS[i].value != "")
				cf.IPv6_Pri_DNS[i].value = parseInt(cf.IPv6_Pri_DNS[i].value, 16).toString(16);

			if(cf.IPv6_Pri_DNS[i].value == "")
			{
				ip_trans_array[2] = ip_trans_array[2] + "0" + ":";
				cf.ipv6_primary_dns_fixed.value = cf.ipv6_primary_dns_fixed.value + "0" + ":";/* to fix bug 24430  bug33138*/
			}
			else
			{
				ip_trans_array[2] = ip_trans_array[2] + cf.IPv6_Pri_DNS[i].value + ":";
				cf.ipv6_primary_dns_fixed.value = cf.ipv6_primary_dns_fixed.value + cf.IPv6_Pri_DNS[i].value + ":";
			}
		}


		/*------------------------------Second DNS------------------------------------*/
		if( check_sec_dns ){// to fix bug 26923
			if(check_ipv6_DNS_address(cf.IPv6_Sec_DNS[i].value) == false)
			{
				alert("$invalid_ipv6_second_dns_hex");
				return false;
			}

			if(cf.IPv6_Sec_DNS[i].value != "")
				cf.IPv6_Sec_DNS[i].value = parseInt(cf.IPv6_Sec_DNS[i].value, 16).toString(16);

			if(cf.IPv6_Sec_DNS[i].value == "")
			{
				ip_trans_array[3] = ip_trans_array[3] + "0" + ":";
				cf.ipv6_second_dns_fixed.value = cf.ipv6_second_dns_fixed.value + "0" + ":";/* to fix bug 24430  bug33138*/
			}
			else
			{
				ip_trans_array[3] = ip_trans_array[3] + cf.IPv6_Sec_DNS[i].value + ":";
				cf.ipv6_second_dns_fixed.value = cf.ipv6_second_dns_fixed.value + cf.IPv6_Sec_DNS[i].value + ":";
			}
		}


		/*---------------------------------LAN----------------------------------------*/
		if(check_ipv6_IP_address(cf.IPv6_lan[i].value) == false)
		{
			alert("$invalid_lan_ipv6_hex");
			return false;
		}
	
		if(cf.IPv6_lan[i].value != "")
			cf.IPv6_lan[i].value = parseInt(cf.IPv6_lan[i].value, 16).toString(16);

		if(cf.IPv6_lan[i].value == "")
		{
			ip_trans_array[4] = ip_trans_array[4] + "0" + ":";
			cf.ipv6_lan_ip_fixed.value = cf.ipv6_lan_ip_fixed.value + "0" + ":";/* to fix bug 24430 */
		}
		else
		{
			ip_trans_array[4] = ip_trans_array[4] + cf.IPv6_lan[i].value + ":";
			cf.ipv6_lan_ip_fixed.value = cf.ipv6_lan_ip_fixed.value + cf.IPv6_lan[i].value + ":";
		}
	}

        if(check_ipv6_IP_address(cf.IPv6_wan_prefix.value) == false)
        {
                alert("$invalid_wan_ipv6_addr_pre_length");
                return false;
        }
        if(check_ipv6_IP_address(cf.IPv6_lan_prefix.value) == false)
        {
                alert("$invalid_lan_ipv6_addr_pre_length");
                return false;
        }
	/****************************************************************************************************/

	/*------------------------------WAN-------------------------------------*/
	var str = cf.ipv6_wan_ip_fixed.value;
	cf.ipv6_wan_ip_fixed.value = str.substring(0, str.length-1);

	if(cf.ipv6_wan_ip_fixed.value == ":::::::")
	{
		cf.ipv6_wan_ip_fixed.value = "";
	}
	else if(cf.ipv6_wan_ip_fixed.value == "0:0:0:0:0:0:0:0")
	{
		alert("$invalid_wan_ipv6_addr");
		return false;
	}
	else if(ip_trans_array[0] == "0:0:0:0:0:0:0:0:")
	{
		alert("$invalid_wan_ipv6_addr");
		return false;
	}

	cf.ipv6_wan_fixed_prefix.value = cf.IPv6_wan_prefix.value;
	if(cf.ipv6_wan_ip_fixed.value == "")
	{
		alert("$wan_lack_ipv6_pre_length");
		return false;
	}
	if(cf.ipv6_wan_fixed_prefix.value == "" || parseInt(cf.ipv6_wan_fixed_prefix.value, 10) > 126 || parseInt(cf.ipv6_wan_fixed_prefix.value, 10) < 4)
	{
		alert("$invalid_wan_ipv6_addr_pre_length");
		return false;
	}
	//for bug 20078, and special checking temply
	if(check_addr_legality(cf.ipv6_wan_ip_fixed.value) == false)
	{
		alert("$invalid_wan_ipv6_addr");
		return false;
	}

	/*-----------------------------gateway----------------------------------*/
	str = cf.ipv6_wan_gw_fixed.value;
	cf.ipv6_wan_gw_fixed.value = str.substring(0, str.length-1);

	if(cf.ipv6_wan_gw_fixed.value == ":::::::")
	{
		cf.ipv6_wan_gw_fixed.value = "";
	}
	else if(cf.ipv6_wan_gw_fixed.value == "0:0:0:0:0:0:0:0")
	{
		alert("$invalid_ipv6_gateway");
		return false;
	}
	else if(ip_trans_array[1] == "0:0:0:0:0:0:0:0:")
	{
		alert("$invalid_ipv6_gateway");
		return false;
	}
	//for bug 20078, and special checking temply
	if(cf.ipv6_wan_gw_fixed.value != "")
	{
		if(check_addr_legality(cf.ipv6_wan_gw_fixed.value) == false)
		{
			alert("$invalid_ipv6_gateway");
			return false;
		}
	}

	/*-----------------------------primary DNS----------------------------------*/
	str = cf.ipv6_primary_dns_fixed.value;
	cf.ipv6_primary_dns_fixed.value = str.substring(0, str.length-1);
	
	if(cf.ipv6_primary_dns_fixed.value == ":::::::")
	{
		cf.ipv6_primary_dns_fixed.value = "";
	}
	else if(cf.ipv6_primary_dns_fixed.value == "0:0:0:0:0:0:0:0")
	{
		alert("$invalid_ipv6_primary_dns");
		return false;
	}
	else if(ip_trans_array[2] == "0:0:0:0:0:0:0:0:")
        {
		alert("$invalid_ipv6_primary_dns");
                return false;
        }
	//for bug 20078, and special checking temply
	if(cf.ipv6_primary_dns_fixed.value != "")
	{
		if(check_addr_legality(cf.ipv6_primary_dns_fixed.value) == false)
		{
			alert("$invalid_ipv6_primary_dns");
			return false;
		}
		else if(cf.ipv6_primary_dns_fixed.value == cf.ipv6_wan_ip_fixed.value )/* to fix bug 26445 */
		{
			alert("$invalid_wan_primary_second_dns");
			return false;
		}
	}

	/*-----------------------------second DNS-------------------------------------*/
	str = cf.ipv6_second_dns_fixed.value;
	cf.ipv6_second_dns_fixed.value = str.substring(0, str.length-1);

	if(cf.ipv6_second_dns_fixed.value == ":::::::")
	{
		cf.ipv6_second_dns_fixed.value = "";
	}
	else if(cf.ipv6_second_dns_fixed.value == "0:0:0:0:0:0:0:0")
	{
		alert("$invalid_ipv6_second_dns");
		return false;
	}
	else if(ip_trans_array[3] == "0:0:0:0:0:0:0:0:")
	{
		alert("$invalid_ipv6_second_dns");
		return false;
	}
	//for bug 20078, and special checking temply
	if(cf.ipv6_second_dns_fixed.value != "")
	{
		if(check_addr_legality(cf.ipv6_second_dns_fixed.value) == false)
		{
			alert("$invalid_ipv6_second_dns");
			return false;
		}
		else
			if(cf.ipv6_second_dns_fixed.value == cf.ipv6_wan_ip_fixed.value)
			{
				alert("$invalid_wan_primary_second_dns");
				return false;
			}
			else
				if(cf.ipv6_primary_dns_fixed.value == cf.ipv6_second_dns_fixed.value )
				{
					alert("$invalid_primary_second_dns");
					return false;
				}
	}

	/*-------------------------------LAN--------------------------------------*/
	str = cf.ipv6_lan_ip_fixed.value;
	cf.ipv6_lan_ip_fixed.value = str.substring(0, str.length-1);

	if(cf.ipv6_lan_ip_fixed.value == ":::::::")
	{
		cf.ipv6_lan_ip_fixed.value = "";
	}
	else if(cf.ipv6_lan_ip_fixed.value == "0:0:0:0:0:0:0:0")
	{
		alert("$invalid_lan_ipv6_addr");
		return false;
	}
	else if(ip_trans_array[4] == "0:0:0:0:0:0:0:0:")
	{
		alert("$invalid_lan_ipv6_addr");
		return false;
	}

	cf.ipv6_lan_fixed_prefix.value = cf.IPv6_lan_prefix.value;
	if(cf.ipv6_lan_ip_fixed.value == "")
	{
		alert("$lan_lack_ipv6_pre_length");
		return false;
	}
	if(cf.IPv6_lan_prefix.value == "" || parseInt(cf.IPv6_lan_prefix.value, 10) > 126 || parseInt(cf.IPv6_lan_prefix.value, 10) < 4)
	{
		alert("$invalid_lan_ipv6_addr_pre_length");
		return false;
	}
	//for bug 20078, and special checking temply
	if(check_addr_legality(cf.ipv6_lan_ip_fixed.value) == false)
	{
		alert("$invalid_lan_ipv6_addr");
		return false;
	}

	/*************check IP confilict****************/
	var wan_ip = ip_trans_array[0];
	var gate_way = ip_trans_array[1];
	var primary_dns = ip_trans_array[2];
	var second_dns = ip_trans_array[3];
	var lan_ip = ip_trans_array[4];
	if(wan_ip!="" && (wan_ip.toUpperCase()==lan_ip.toUpperCase()))
	{
		alert("$same_ipv6_lan_wan_ip");
		return false;
	}
	if(wan_ip!="" && (wan_ip==primary_dns||wan_ip==second_dns))
	{
		alert("$invalid_wan_ipv6");
		return false;
	}
	if(lan_ip!="" && (lan_ip==primary_dns||lan_ip==second_dns))
	{
		alert("$invalid_lan_ipv6");
		return false;
	}
	/***********************************************/


	/* save IPv6 Filtering */
        if(cf.IPv6Filtering[0].checked == true)
        {
                cf.ipv6_hidden_filtering.value = "0";
        }
        else if(cf.IPv6Filtering[1].checked == true)
        {
                cf.ipv6_hidden_filtering.value = "1";
        }
	return true;
} 

