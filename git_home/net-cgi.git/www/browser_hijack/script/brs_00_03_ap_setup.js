
function initPage()
{
	var head_tag  = document.getElementsByTagName("h1");

	var btns_container_div = document.getElementById("btnsContainer_div");
	if( master == "admin" )
	{
		btns_container_div.onclick = function() 
		{
			return checkap(document.forms[0]);
		}
	}

	loadvalue()
	//show firmware version
    showFirmVersion("none");
}

addLoadEvent(initPage);

function ap_show()
{
		var cf=document.forms[0];
		
		document.getElementById("static_ip").style.display="";
}

function ap_notshow()
{
		var cf=document.forms[0];
		
		document.getElementById("static_ip").style.display="none";
}
function loadvalue()
{
	var form=document.forms[0];

	form.APaddr1.value=form.APaddr2.value=form.APaddr3.value=form.APaddr4.value="0"
	form.APmask1.value=form.APmask2.value=form.APmask3.value=form.APmask4.value="0";
	form.APgateway1.value=form.APgateway2.value=form.APgateway3.value=form.APgateway4.value="0";
	form.APDAddr1.value=form.APDAddr2.value=form.APDAddr3.value=form.APDAddr4.value="0";
	form.APPDAddr1.value=form.APPDAddr2.value=form.APPDAddr3.value=form.APPDAddr4.value="";

}

function check_static_ip_mask_gtw(form)
{
	if(checkipaddr(form.hid_ap_ipaddr.value)==false || is_sub_or_broad(form.hid_ap_ipaddr.value, form.hid_ap_ipaddr.value, form.hid_ap_subnet.value) == false)
	{
		alert(bh_invalid_ip);
		return false;
	}
	if(checksubnet(form.hid_ap_subnet.value, 0)==false)
	{
		alert(bh_invalid_mask);
		return false;
	}
	if(checkgateway(form.hid_ap_gateway.value)==false)
	{
		alert(bh_invalid_gateway);
		return false;
	}

	if( isSameIp(form.hid_ap_ipaddr.value, form.hid_ap_gateway.value) == true )
	{
		alert(bh_invalid_gateway);
		return false;
	}

	return true;
}

function check_static_dns( wan_assign )
{
	var form=document.forms[0];
	form.ap_dnsaddr1.value=form.APDAddr1.value+'.'+form.APDAddr2.value+'.'+form.APDAddr3.value+'.'+form.APDAddr4.value;
    form.ap_dnsaddr2.value=form.APPDAddr1.value+'.'+form.APPDAddr2.value+'.'+form.APPDAddr3.value+'.'+form.APPDAddr4.value;
	form.hid_ap_ipaddr.value=form.APaddr1.value+'.'+form.APaddr2.value+'.'+form.APaddr3.value+'.'+form.APaddr4.value;

	if(form.ap_dnsaddr1.value=="...")
		form.ap_dnsaddr1.value="";

	if(form.ap_dnsaddr2.value=="...")
		form.ap_dnsaddr2.value="";
	if( check_DNS(form.ap_dnsaddr1.value,form.ap_dnsaddr2.value,wan_assign,form.hid_ap_ipaddr.value))
		return true;
	else
		return false;
}

function checkap(form) //for bug 30286
{
	form.hid_ap_ipaddr.value=form.APaddr1.value+'.'+form.APaddr2.value+'.'+form.APaddr3.value+'.'+form.APaddr4.value;
	form.hid_ap_subnet.value=form.APmask1.value+'.'+form.APmask2.value+'.'+form.APmask3.value+'.'+form.APmask4.value;
	form.hid_ap_gateway.value=form.APgateway1.value+'.'+form.APgateway2.value+'.'+form.APgateway3.value+'.'+form.APgateway4.value;
	form.ap_dnsaddr1.value=form.APDAddr1.value+'.'+form.APDAddr2.value+'.'+form.APDAddr3.value+'.'+form.APDAddr4.value;
	form.ap_dnsaddr2.value=form.APPDAddr1.value+'.'+form.APPDAddr2.value+'.'+form.APPDAddr3.value+'.'+form.APPDAddr4.value;


	if( wps_progress_status == "2" || wps_progress_status == "3" || wps_progress_status == "start" )
	{
		alert(bh_wps_in_progress);
		return false;
	}

	form.hid_enable_apmode.value="1";
			   
	if(form.dyn_get_ip[1].checked == true)
	{
		if(check_static_ip_mask_gtw(form)==false)
			return false;			
		if(check_static_dns(!(form.dyn_get_ip.checked)) == false)
			return false;	
		form.hid_dyn_get_ip.value="0";//for static
	}
	else
		form.hid_dyn_get_ip.value="1"; //for dynamic

	form.submit();
	//location.href="BRS_00_04_ap_wait.html";
}

