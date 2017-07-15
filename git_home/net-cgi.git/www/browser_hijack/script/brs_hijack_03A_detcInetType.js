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

	setTimeout("loadValue()", 4000);
}

function loadValue()
{
	if(port_status == "0")
	{
		this.location.href = "BRS_03A_A_noWan.html";
	}
	if(checking_result > 10000)
	{
		pageRedirect()
	}
}

function pageRedirect()
{
	switch(checking_result)
	{
		case "10002":
			this.location.href = "BRS_03A_B_pppoe.html";
			break;
		case "10007":
			this.location.href = "BRS_03A_C_pptp.html";
			break;
                case "10010":
                        this.location.href = "BRS_03A_F_l2tp.html";
			break;
		case "10006":
//bug 35045		this.location.href = "BRS_03A_D_bigpond.html";
			this.location.href = "BRS_03A_E_IP_problem.html";
			break;
		case "10003":
			this.location.href = "BRS_03A_E_IP_problem.html";
			break;
		case "10004":
                        this.location.href = "BRS_03A_E_IP_problem.html";
                        break;
		case "10001":
			this.location.href = "BRS_04_B_checkNet_ping.html";
			break;
		default:
			this.location.href = "BRS_03A_A_noWan.html";
			break;
	}
}

addLoadEvent(initPage);


