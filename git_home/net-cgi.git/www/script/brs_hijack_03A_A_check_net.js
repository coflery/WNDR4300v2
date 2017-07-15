function initPage()
{
        var head_tag = document.getElementsByTagName("h1");

        var head_text = document.createTextNode(bh_detecting_connection);
        head_tag[0].appendChild(head_text);

        var paragraph = document.getElementsByTagName("p");

        var paragraph_text = document.createTextNode(bh_plz_wait_process);
        paragraph[0].appendChild(paragraph_text);


        //show Fireware Version
        showFirmVersion("none");
	
	setTimeout("loadValue()", 3000);
}

function loadValue()
{
	var forms = document.getElementsByTagName("form");
	var cf = forms[0];

	if(checking_result == "failed")	//failed
	{
		if( top.is_ru_version == 1)
			this.location.href="RU_check_wan_port.html";
		else
			cf.submit();
	}
	else if(checking_result == "success") //success
		this.location.href = "BRS_success.html";
}

addLoadEvent(initPage);
