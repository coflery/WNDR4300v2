function initPage()
{
	var head_tag  = document.getElementsByTagName("h1");
	var connect_text = document.createTextNode(bh_internet_checking);
	head_tag[0].appendChild(connect_text);

        var image = document.getElementById("waiting_img");
        image.setAttribute("src", "image/wait30.gif");
	
	loadValue();
}

function loadValue()
{
	var forms = document.getElementsByTagName("form");
	var cf = forms[0];

	if(ping_result == "failed")	//failed
	{
		if (((top.netgear_region.toUpperCase() == "WW" || top.netgear_region == "") && top.gui_region == "Russian" ) || top.netgear_region.toUpperCase() == "RU")
			this.location.href = "RU_welcome.htm";
		else
			this.location.href = "BRS_05_networkIssue.html";
	}
	else if(ping_result == "success") //success
		this.location.href = "BRS_success.html";
	setTimeout("loadValue();", 1000);
}

addLoadEvent(initPage);
