function initPage()
{
	//head text
	var head_tag = document.getElementsByTagName("h1");
	var head_text = document.createTextNode(bh_plz_wait_moment);
	head_tag[0].appendChild(head_text);

	setTimeout("loadValue()", 60000);
}

function loadValue()
{
	this.location.href="BRS_success.html";
}

addLoadEvent(initPage);
