function setRemote()
{
	var cf = document.forms[0];
	var i;
	for( i=0; i<cf.Romte_IP.length; i++)
	{
		if( cf.remote[0].checked == true )
		{
			cf.Romte_IP[i].disabled = true;
		}
		else if( cf.remote[1].checked == true )
		{
			cf.Romte_IP[i].disabled = false;
		}
	}
}	

function check_ipv6_6to4(cf)
{
	if(enable_vpn == "1")
	{
		alert("$no_enable_ipv6");
		return false;
	}
	var i;

	if(internet_basic_type == 0)	//not dhcp
	{	
		if((internet_ppp_type == 0) || (internet_ppp_type == 1) || (internet_ppp_type == 4))	//(pptp, 1),(pppoe, 0),(L2tp,4)
		{
			if(confirm("$ipv6_6to4_warning") == false)
			{
				return false;
			}	
		}
	}

	/* Remote 6to4 Relay Router */
	if(cf.remote[1].checked == true)
	{
		cf.ipv6_hidden_6to4_relay.value = "";
		for( i=0; i<cf.Romte_IP.length; i++)
		{
			if( i < (cf.Romte_IP.length-1) )
			{
				cf.ipv6_hidden_6to4_relay.value = cf.ipv6_hidden_6to4_relay.value + cf.Romte_IP[i].value + ".";
			}
			else if( i == (cf.Romte_IP.length-1) )
			{
				cf.ipv6_hidden_6to4_relay.value = cf.ipv6_hidden_6to4_relay.value + cf.Romte_IP[i].value;
			}
		}
		if( checkipaddr(cf.ipv6_hidden_6to4_relay.value) == false )
		{
			alert("$invalid_ip");
			return false;
		}
	}
	else
		cf.ipv6_hidden_6to4_relay.value = ipv6_6to4_relay_ip;
	
	if(top.ipv6_dns_manual == 1)
	{
		if(checkIPv6DNS(cf) == false)
			return false;
	}

	if( ipv6_save_common(cf) == false )
	{
		return false;
	}
	return true;
}

