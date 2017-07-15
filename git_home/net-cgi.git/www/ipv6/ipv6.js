/*
 * bug 23820: The displayed address has no underline

ipv6 ip address have 8 cell, one cell have 4 char
the ipv6 address format should follow RFC 4291: the syntax compressing the zeros
where the prefix length (64, in this example) is displayed after the slash '/'.. The prefix part of the
IPv6 address is underlined, and, if the prefix length is not an integral multiple of 4, the underline 
extends to the hex digit containing the last prefix bit 
(for example, <u>2001:b11</u>2::2314:6d64:dcd5:5c19:40ed/25). 
*/
function show_ipv6_ip(ip)
{
/*
		use 2001:b112::6d64:cd5:5c19:40ed/88 for example
                full ipv6 address is:
                2001:b112:0000:0000:6d64:0cd5:5c19:40ed/88
                add underline, it should show as:
                <u>2001:b112::6d64:c</u>d5:5c19:40ed/88
*/
	var show_ip, cell_ip, pre_ip, suf_ip;
	var length, cell_num, char_num, blank_num, blank_lenth;
	var i;
	show_ip = ip.split('/');
	
	if( show_ip.length > 1)
	{
		if( parseInt(show_ip[1]) >= 128 )
		{// all ip are prefix
			document.write('<u>'+show_ip[0]+'</u>'+'/'+show_ip[1]);
			return;
		}
		
		length = Math.ceil(parseInt(show_ip[1])/4);	// prefix char length: 88/4 = 22
		cell_num = Math.floor(length/4); //cell_num in full address is ( 22/4 = 5), array begin with 0.
		char_num = length % 4;	// 22 % 4 = 2: the last cell in prefix have 2 char

		cell_ip = show_ip[0].split(':');		
		blank_num = -1;
		for( i=0; i< cell_ip.length; i++)
		{
			if(cell_ip[i] == "")
				blank_num = i;	// find the "::" position, one ipv6 most have one "::"
		}
		blank_length = 8 - cell_ip.length;	// "::" contains many cells, at this case, :: contains two cells 

		// find cell_num in compressing the zeros ipv6 address. cell_num = 4
		if( cell_num > blank_num + blank_length )
			cell_num = cell_num - blank_length;
		else if( blank_num < cell_num && cell_num <=blank_num + blank_length)
			cell_num = blank_num;

		char_num = char_num - (4 - cell_ip[cell_num].length); // cell_num[4] = "cd5" = "0cd5",so char_num should Minus one

		// get the prefix before cell_ip[cell_num] (:cd5:)
		pre_ip = "";
		for( i=0; i< cell_num; i++)
		{
			pre_ip = pre_ip +cell_ip[i]+':';
		}
	
		// get the prefix in cell_ip[cell_num] (cd5, prefix = c, suffix = d5)
		if( char_num > 0)
		{	

			pre_ip = pre_ip + cell_ip[cell_num].substring(0, char_num);

			suf_ip = cell_ip[cell_num].substring(char_num, cell_ip[cell_num].length);
		}
		else
			suf_ip = cell_ip[cell_num];
		
		// get the last suffix
		for( i = cell_num+1; i< cell_ip.length; i++)
		{
			suf_ip = suf_ip + ':' + cell_ip[i];
		}

		// if at the end of prefix have ':', move ':' to suffix
		for( i = pre_ip.length - 1; i >= 0; i--)
		{
			if( pre_ip[i] == ':' )
			{
				suf_ip = ':' + suf_ip;
			}
			else
				break;
		}
		pre_ip = pre_ip.substring(0, i+1);
		
		document.write('<u>'+pre_ip+'</u>'+suf_ip+'/'+show_ip[1]);
	}
	else
	{
		document.write(ip);
	}
}


function ipv6_write_ip(ipv6_ip_addr)
{
	if(ipv6_ip_addr != "")
		ipv6_ip_addr = remove_space(ipv6_ip_addr);

	if(ipv6_ip_addr == "")
	{
		document.write("<TR><TD nowrap>$spacebar"+"$ipv6_not_available</TD></TR>");
	}
	else
	{
		var each_ip = ipv6_ip_addr.split(' ');
		var i;

		for(i=0; i<each_ip.length; i++)
		{
			document.write("<TR><TD nowrap>$spacebar")
			show_ipv6_ip(each_ip[i])
			document.write("</TD></TR>");
		}
	} 
}

function ipv6_load_common(cf)
{
	/* IP Address Assignment */
        if( ipv6_ip_assign == "1" )
        {
                cf.ipv6_lan_ip_assign[0].checked = true;
        }
        else if( ipv6_ip_assign == "0" )
        {
                cf.ipv6_lan_ip_assign[1].checked = true;
        }

        /* Use This Interface ID  */
        if(ipv6_interface_type == "1")
        {
                cf.enable_interface.checked = true;
        }
        else
        {
                cf.enable_interface.checked = false;
        }
        set_interface();
        var interface_id_array = ipv6_interface_id.split(':');
        var i;
        for( i=0; i<interface_id_array.length; i++ )
        {
                cf.IP_interface[i].value = interface_id_array[i];
        }

	/* IPv6 Filtering */
	if(ipv6_cone_fitering == 0)
	{
		cf.IPv6Filtering[0].checked = true;
	}
	else if(ipv6_cone_fitering == 1)
	{
		cf.IPv6Filtering[1].checked = true;
	}
}

function ipv6_save_common(cf)
{
	var i;
        cf.ipv6_hidden_interface_id.value = "";

	/* Use This Interface ID */
	if(enable_vpn == "1")
	{
		alert("$no_enable_ipv6");
		return false;
	}
        if( cf.enable_interface.checked == true )
        {
			cf.ipv6_hidden_enable_interface.value = "1";
			for( i=0; i<cf.IP_interface.length; i++ )
			{
				if( check_ipv6_IP_address(cf.IP_interface[i].value) == false )
				{
					alert("$ipv6_invalid_interface_id");
					return false;
				}
				if( cf.IP_interface[i].value == "" )
				{
					cf.IP_interface[i].value = "0";
				}
				if( i < (cf.IP_interface.length-1) )
				{
					cf.ipv6_hidden_interface_id.value = cf.ipv6_hidden_interface_id.value + cf.IP_interface[i].value + ":";
				}
				else if( i == (cf.IP_interface.length-1) )
				{
					//to fix bug29794:"interface ID" can be set to "x:x:x:0",and it wi    ll make ipv6 address of lan to be a network segment.
					if(cf.IP_interface[i].value == "0")
					{
						alert("$ipv6_invalid_interface_id");
						return false;
					}
					else
						cf.ipv6_hidden_interface_id.value = cf.ipv6_hidden_interface_id.value + cf.IP_interface[i].value;
                }
			}
        }
        else
        {
                cf.ipv6_hidden_enable_interface.value = "0";
        }

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

function set_interface()
{
        var cf = document.forms[0];
        var i;
	for( i=0; i<cf.IP_interface.length; i++)
	{
		if( cf.enable_interface.checked == true )
        	{
                	cf.IP_interface[i].disabled = false;
        	}
       	 	else if( cf.enable_interface.checked == false )
        	{	
                	cf.IP_interface[i].disabled = true;
        	}
	}
}
//bug 26966 dns server ipv6 address should be able to leave these fields unspecified
function check_ipv6_DNS_address(ipv6_dns_value)
{
	var i;

	if(ipv6_dns_value != "")
	{
		for(i=0; i<ipv6_dns_value.length;)
		{
			if((ipv6_dns_value.charAt(i)>="0" && ipv6_dns_value.charAt(i)<="9") || (ipv6_dns_value.charAt(i)>="a" && ipv6_dns_value.charAt(i)<="f") || (ipv6_dns_value.charAt(i)>="A" && ipv6_dns_value.charAt(i)<="F"))
			{
				i++;
			}
			else
			{
				return false;
			}
		}
	}

	return true;	
}

function check_ipv6_IP_address(ipv6_ip_value)
{
	var i;

	if(ipv6_ip_value != "")
	{
		for(i=0; i<ipv6_ip_value.length;)
		{
			if((ipv6_ip_value.charAt(i)>="0" && ipv6_ip_value.charAt(i)<="9") || (ipv6_ip_value.charAt(i)>="a" && ipv6_ip_value.charAt(i)<="f") || (ipv6_ip_value.charAt(i)>="A" && ipv6_ip_value.charAt(i)<="F"))
			{
				i++;
			}
			else
			{
				return false;
			}
		}
	}
	// 2lines for bug 26010
	//else//bug 23597:the address can't blank
	//	return false;

	return true;	
}

function change_conn_type_name(conn_type)
{
	var type;
	if(conn_type=="Detecting...")
		type="$ipv6_detecting";
	else if(conn_type=="6to4 Tunnel")
		type="$ipv6_6to4_tunnel";
	else if(conn_type=="Pass Through")
		type="$ipv6_pass_through";
	else if(conn_type=="Auto Config")
		type="$ipv6_auto_config";
	else if(conn_type=="DHCP")
		type="$router_status_dhcp";
	else
		type=conn_type;
	return type;
}

function setIPv6DNS(cf)
{
        var dflag = cf.IPv6DNSAssign[0].checked;
	for(i=0; i< cf.IPv6_Pri_DNS.length ; i++)
	{
		cf.IPv6_Pri_DNS[i].disabled = dflag;
		cf.IPv6_Sec_DNS[i].disabled = dflag;
	}
}

function check_addr_legality(value)
{
	if(value.charAt(0).toLowerCase()=="f" && value.charAt(1).toLowerCase()=="f")
		return false;

	if(value==":::::::1" || value=="0:0:0:0:0:0:0:1")
		return false;

	return true;
}

function checkIPv6DNS(cf)
{
	if(cf.IPv6DNSAssign[1].checked)
	{
		cf.ipv6_hidden_primary_dns.value = "";
		cf.ipv6_hidden_second_dns.value = "";
		var check_pri_dns = 1;/* 1: Primary DNS is not empty;*/
		var check_sec_dns = 1;/* 1: Second DNS is not empty;*/
		var pri_dns = "";
		var sec_dns = "";

		for( i=0; i<cf.IPv6_Pri_DNS.length; i++ )
		{
			pri_dns = pri_dns + cf.IPv6_Pri_DNS[i].value;
			sec_dns = sec_dns + cf.IPv6_Sec_DNS[i].value;
		}
		if( pri_dns == "" )
			check_pri_dns = 0;
		if( sec_dns == "")
			check_sec_dns = 0;

		if(check_pri_dns == 0 && check_sec_dns == 0)
		{
			alert("$dns_must_specified");
			return false;
		}

		for( i=0; i<cf.IPv6_Pri_DNS.length; i++ )
		{
			if( check_pri_dns )
			{
				if(check_ipv6_DNS_address(cf.IPv6_Pri_DNS[i].value) == false)
				{
					alert("$invalid_ipv6_primary_dns_hex");
					return false;
				}

				if(cf.IPv6_Pri_DNS[i].value != "")
					cf.IPv6_Pri_DNS[i].value = parseInt(cf.IPv6_Pri_DNS[i].value, 16).toString(16);

				if(cf.IPv6_Pri_DNS[i].value == "")
					cf.ipv6_hidden_primary_dns.value = cf.ipv6_hidden_primary_dns.value + "0" + ":";
				else
					cf.ipv6_hidden_primary_dns.value = cf.ipv6_hidden_primary_dns.value + cf.IPv6_Pri_DNS[i].value + ":";
			}

			if( check_sec_dns )
			{
				if(check_ipv6_DNS_address(cf.IPv6_Sec_DNS[i].value) == false)
				{
					alert("$invalid_ipv6_second_dns_hex");
					return false;
				}

				if(cf.IPv6_Sec_DNS[i].value != "")
					cf.IPv6_Sec_DNS[i].value = parseInt(cf.IPv6_Sec_DNS[i].value, 16).toString(16);

				if(cf.IPv6_Sec_DNS[i].value == "")
					cf.ipv6_hidden_second_dns.value = cf.ipv6_hidden_second_dns.value + "0" + ":";
				else
					cf.ipv6_hidden_second_dns.value = cf.ipv6_hidden_second_dns.value + cf.IPv6_Sec_DNS[i].value + ":";
			}
		}

		var str = cf.ipv6_hidden_primary_dns.value;
		cf.ipv6_hidden_primary_dns.value = str.substring(0, str.length-1);
		if(cf.ipv6_hidden_primary_dns.value == ":::::::")
			cf.ipv6_hidden_primary_dns.value = "";
		else if(cf.ipv6_hidden_primary_dns.value == "0:0:0:0:0:0:0:0")
		{
			alert("$invalid_ipv6_primary_dns");
			return false;
		}

		if(cf.ipv6_hidden_primary_dns.value != "")
		{
			if(check_addr_legality(cf.ipv6_hidden_primary_dns.value) == false)
			{
				alert("$invalid_ipv6_primary_dns");
				return false;
			}
		}

		str = cf.ipv6_hidden_second_dns.value;
		cf.ipv6_hidden_second_dns.value = str.substring(0, str.length-1);
		
		if(cf.ipv6_hidden_second_dns.value == ":::::::")
			cf.ipv6_hidden_second_dns.value = "";
		else if(cf.ipv6_hidden_second_dns.value == "0:0:0:0:0:0:0:0")
		{
			alert("$invalid_ipv6_second_dns");
			return false;
		}

		if(cf.ipv6_hidden_second_dns.value != "")
		{
			if(check_addr_legality(cf.ipv6_hidden_second_dns.value) == false)
			{
				alert("$invalid_ipv6_second_dns");
				return false;
			}
			else if(cf.ipv6_hidden_primary_dns.value == cf.ipv6_hidden_second_dns.value )
			{
				alert("$invalid_primary_second_dns");
				return false;
			}
		}
	}
}

function load_ipv6_dns(cf)
{
	if (get_dns_assign == '1')
		cf.IPv6DNSAssign[1].checked = true;
	else
		cf.IPv6DNSAssign[0].checked = true;

	setIPv6DNS(cf);

	if(ipv6_get_dns1 !="")
	{
		var ipv6_primary_dns = ipv6_get_dns1.split(":");
		for( i=0; i<ipv6_primary_dns.length; i++ )
		{
			if(ipv6_primary_dns[i]=="")
				cf.IPv6_Pri_DNS[i].value = "0";
			else
				cf.IPv6_Pri_DNS[i].value = ipv6_primary_dns[i];
		}
	}

	if(ipv6_get_dns2 != "")
	{
		var ipv6_second_dns = ipv6_get_dns2.split(":");
		for( i=0; i<ipv6_second_dns.length; i++ )
		{
			if(ipv6_second_dns[i]=="")
				cf.IPv6_Sec_DNS[i].value = "0";
			else
				cf.IPv6_Sec_DNS[i].value = ipv6_second_dns[i];
		}
	}
}
