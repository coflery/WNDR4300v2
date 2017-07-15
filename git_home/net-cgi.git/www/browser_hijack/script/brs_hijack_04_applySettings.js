function initPage()
{
	//head text
	var head_tag = document.getElementsByTagName("h1");
	var head_text = document.createTextNode(bh_apply_connection);
	head_tag[0].appendChild(head_text);
	
	//paragraph
	var paragraph = document.getElementsByTagName("p");
	
	var paragraph_text = document.createTextNode(bh_plz_waite_apply_connection);
	paragraph[0].appendChild(paragraph_text);

	//show Fireware Version
	showFirmVersion("none");

	if(top.location.href.indexOf("BRS_index.htm") > -1){
		if(top.location.href.indexOf("routerlogin.net") > -1){
			setTimeout("top.location.href='http://routerlogin.com/BRS_index.htm';", 60000);
		}else{
			setTimeout("top.location.href='http://routerlogin.net/BRS_index.htm';", 60000);
		}
	}else{
		setTimeout("this.location.href='BRS_04_applySettings_ping.html';", 60000);
	}
}

addLoadEvent(initPage);
