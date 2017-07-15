function click_ddns(form)
{
	var new_sysDNSHost = str_replace(old_sysDNSHost);
	var new_sysDNSUser = str_replace(old_sysDNSUser);
	var new_sysDNSPassword = str_replace(old_sysDNSPassword);
	if (form.sysDNSActive.checked)
		form.ddns_enabled.value = "1";
	else
		form.ddns_enabled.value = "0";

	if(ddns_wildcards_flag==1 && form.sysDNSProviderlist.value == "www/var/www.3322.org")
	{
		if (form.sysDNSWildCard.checked)
			form.wildcards_enabled.value="1";
		else
			form.wildcards_enabled.value="0";
	}
	else
		form.wildcards_enabled.value="0";
	form.hidden_sysDNSHost.value = form.sysDNSHost.value;
	if (form.sysDNSActive.checked)
	{
		if(form.sysDNSHost.value=="" && form.sysDNSProviderlist.value != "www/var/www.oray.cn")
		{
			alert("$hostname_null");
			return false;
		}
		if(form.sysDNSUser.value=="")
		{
			alert("$user_name_null");
			return false;
		}
		if(form.sysDNSPassword.value=="")
		{
			alert("$password_null");
			return false;
		}
		if(form.sysDNSProviderlist.value != "www/var/www.oray.cn")//hostname
		{
			for (i=0;i<form.sysDNSHost.value.length;i++)
			{
				if(isValidDdnsHost(form.sysDNSHost.value.charCodeAt(i))==false)
				{
					alert("$hostname_error");
					return false;
				}
			}
		}
		if(form.sysDNSProviderlist.value == "www/var/www.oray.cn")//username
		{
			for (i=0;i<form.sysDNSUser.value.length;i++)
			{//37033
				if(isValidDdnsOrayUserName(form.sysDNSUser.value.charCodeAt(i))==false)
				{
					alert("$user_name_error");
					return false;
				}
			}
		}
		else if(form.sysDNSProviderlist.value == "www/var/www.3322.org")
		{
			if(form.sysDNSUser.value.length <3 || form.sysDNSUser.value.length >30)
			{
				alert("$user_name_error");
				return false;
			}
			for (i=0;i<form.sysDNSUser.value.length;i++)
			{
				if(isValidDdns3322UserName(form.sysDNSUser.value.charCodeAt(i))==false)
				{
					alert("$user_name_error");
					return false;
				}
				if(i==0 || i==form.sysDNSUser.value.length-1)
				{
					if(form.sysDNSUser.value.charCodeAt(i)==95)
					{
						alert("$user_name_error");
						return false;
					}
				}
			}
		}
		else
		{
			for (i=0;i<form.sysDNSUser.value.length;i++)
			{
				if(isValidChar_space(form.sysDNSUser.value.charCodeAt(i))==false)
				{
					alert("$user_name_error");
					return false;
				}
			}
		}
		for (i=0;i<form.sysDNSPassword.value.length;i++)//password
		{
			if(isValidChar_space(form.sysDNSPassword.value.charCodeAt(i))==false)
			{
				alert("$password_error");
				return false;
			}
		}
		if(form.sysDNSProviderlist.value == "www/var/www.3322.org")
		{
			if(form.sysDNSPassword.value.length<6 || form.sysDNSPassword.value.length >30)
			{
				alert("$password_error");
				return false;
			}
		}
		if ((old_endis_ddns == form.ddns_enabled.value) && (new_sysDNSHost == form.sysDNSHost.value) && (new_sysDNSUser == form.sysDNSUser.value) && (new_sysDNSPassword == form.sysDNSPassword.value) && (old_endis_wildcards == form.wildcards_enabled.value))
		{
			alert("$ddns_warning_message");
			return false;
		}
	}
	if (old_endis_ddns != form.ddns_enabled.value || old_sysDNSHost != form.hidden_sysDNSHost.value || old_sysDNSUser != form.sysDNSUser.value || old_sysDNSPassword !=  form.sysDNSPassword.value || old_endis_wildcards != form.wildcards_enabled.value)
		form.change_wan_type.value=0;
	else
		form.change_wan_type.value=1;
	
	if((dns_list != form.sysDNSProviderlist.value || (dns_list == form.sysDNSProviderlist.value && old_sysDNSHost != form.sysDNSHost.value)) && vpn_enable == "1" && form.sysDNSActive.checked == true)
		form.hid_vpn_detect_ddns_change.value = 1;
	else
		form.hid_vpn_detect_ddns_change.value = 0;
	parent.ddns_post_flag = 1;
	return true;
}

function str_replace(str)
{
	str = str.replace(/&#92;/g, "\\").replace(/&lt;/g,"<").replace(/&gt;/g,">").replace(/&#40;/g,"(").replace(/&#41;/g,")").replace(/&#34;/g,'\"').replace(/&#39;/g,"'").replace(/&#35;/g,"#").replace(/&#38;/g,"&");

	return str;
}

function parse_xss_str()
{
	old_sysDNSHost_1 = str_replace(old_sysDNSHost_1);
	old_sysDNSUser_1 = str_replace(old_sysDNSUser_1);
	old_sysDNSPassword_1 = str_replace(old_sysDNSPassword_1);

	old_sysDNSHost_2 = str_replace(old_sysDNSHost_2);
	old_sysDNSUser_2 = str_replace(old_sysDNSUser_2);
	old_sysDNSPassword_2 = str_replace(old_sysDNSPassword_2);

	old_sysDNSHost_3 = str_replace(old_sysDNSHost_3);
	old_sysDNSUser_3 = str_replace(old_sysDNSUser_3);
	old_sysDNSPassword_3 = str_replace(old_sysDNSPassword_3);

	old_sysDNSHost_4 = str_replace(old_sysDNSHost_4);
	old_sysDNSUser_4 = str_replace(old_sysDNSUser_4);
	old_sysDNSPassword_4 = str_replace(old_sysDNSPassword_4);
}

