function convert(value, dataType) {
	switch(dataType) {
    case "int":
		var each_info = value.split(".");
		value = parseInt(each_info[0]) + parseInt(each_info[1]) + parseInt(each_info[2]) + parseInt(each_info[3]);
		return parseInt(value);
		break
    default:
		return value.toString();
  }
}

function compareCols(col, dataType) {
	return function compareTrs(tr1, tr2) {
		value1 = convert(tr1.cells[col].innerHTML, dataType);
		value2 = convert(tr2.cells[col].innerHTML, dataType);
		if (value1 < value2) {
			return -1;
		} else if (value1 > value2) {
			return 1;
		} else {
			return 0;
		}
	};
}

function sortTable(tableId, col, dataType, tag) 
{
	if(enable_block_device == 1 || tag == 1)
	{
		var table = document.getElementById(tableId);
		var tbody = table.tBodies[0];
		var tr = tbody.rows;
		var trValue = new Array();
		for (var i=0; i<tr.length; i++ ) {
			trValue[i] = tr[i];  
		}
		if (tbody.sortCol == col) {
			trValue.reverse(); 
		} else {
			trValue.sort(compareCols(col, dataType));
		}
		var fragment = document.createDocumentFragment(); 
		for (var i=0; i<trValue.length; i++ ) {
			if( i%2== 1 )
				trValue[i].className = "even_line";
			else
				trValue[i].className = "odd_line";
			fragment.appendChild(trValue[i]);
		}
		tbody.appendChild(fragment); 
		tbody.sortCol = col;
	}
}

function load_sortTable(tableId, num, col, dataType, tag)
{
	var j=0;
	var k=0;
	var m=0;
	if(enable_block_device == 1 || tag == 1)
	{
		var table = document.getElementById(tableId);
		var tbody = table.tBodies[0];
		var tr = tbody.rows;
		var trValue1 = new Array();
		var trValue2 = new Array();
		for (var i=0; i<tr.length; i++ ) {
			if(tr[i].cells[num].innerHTML.indexOf("Blocked") > -1)
			{
				trValue1[j] = tr[i]; 
				j++;
			}
		}
		for (var i=0; i<tr.length; i++ ) {
			if(tr[i].cells[num].innerHTML.indexOf("Allowed") > -1)
			{
				trValue2[k] = tr[i]; 
				k++;
			}
		}

		trValue1.sort(compareCols(col, dataType));
		trValue2.sort(compareCols(col, dataType));
		
		var fragment = document.createDocumentFragment(); 
		
		for (var i=0; i<trValue1.length; i++ ) {
			if( i%2== 1 )
				trValue1[i].className = "even_line";
			else
				trValue1[i].className = "odd_line";
			fragment.appendChild(trValue1[i]);
		}
		m = i;
		for (var i=0; i<trValue2.length; i++ ) {

			if( m%2== 1)
				trValue2[i].className = "even_line";
			else
				trValue2[i].className = "odd_line";
					
			fragment.appendChild(trValue2[i]);
			m++;
		}
		tbody.appendChild(fragment);  
		tbody.sortCol = col;
	}	
}

function access_cancel()
{
	location.href="AccessControl_show.htm";	
}

function check_all_device(this_e, start, id)
{
	var cf = document.forms[0];
	var i = start;
	var e;
	var type=this_e.checked;
	
	while( e = eval('document.getElementById("'+id+i+'")'))
	{
		e.checked = type;
		i++;
	}
}

function set_allow_block(cf, flag)
{
	if(flag == 1)
		cf.submit_flag.value = "acc_control_allow";
	else
		cf.submit_flag.value = "acc_control_block";

	var sel_num=0;
	var sel_list="";
	if(access_control_device_num > 0)
	{
		for(i=0;i<access_control_device_num;i++)
		{
			var listName = "check_device"+i;
			if(document.getElementById(listName).checked == true)
			{
				if(flag == 0 && document.getElementById(listName).value.toLowerCase() == wan_remote_mac.toLowerCase())
				{
					alert("$not_block_device_msg");
					return false;
				}
				sel_list+= document.getElementById(listName).value;
				sel_list+= "#";
				sel_num++;
			}
		}
		if(sel_num == 0)
			return false;
		if(flag == 0 && sel_num!=0 && confirm("$acc_block_check") == false)
			return false;
		
		cf.hidden_change_list.value = sel_list;
		cf.hidden_change_num.value = sel_num;
		cf.submit();
	}
	else
		return false;
}

function delete_block()
{
	var cf = document.forms[0];

	cf.submit_flag.value = "delete_acc";

	var sel_list="";
	var count=0;
	if( blocked_no_connect_num > 0 )
	{
		for( i = 0; i <blocked_no_connect_num; i++)
		{
			var listName = "block_not_connect"+i;
			if(document.getElementById(listName).checked == true)
			{
				sel_list+= document.getElementById(listName).value;
				sel_list+= "#";
				count++;
			}
		}
		if(sel_list == "")
			return false;
		if(confirm("$acc_del_check") == false)
			return false;
		cf.hidden_del_list.value = sel_list;
		cf.hidden_del_num.value = count;
		cf.submit();
	}
	else
		return false;
}

function delete_allow()
{
        var cf = document.forms[0];

        cf.submit_flag.value = "delete_acc";
	
	var sel_list="";
	var count=0;
        if( allowed_no_connect_num > 0 )
        {
                for( i = 0; i <allowed_no_connect_num; i++)
                {
			var listName = "allow_not_connect"+i;
			if(document.getElementById(listName).checked == true)
			{
				sel_list+= document.getElementById(listName).value;
				sel_list+= "#";
				count++;
			}
                }
		if(sel_list == "")
			return false;
		if(confirm("$acc_del_check") == false)
			return false;
		cf.hidden_del_list.value = sel_list;
		cf.hidden_del_num.value = count;
		cf.submit();
        }
	else
		return false;
}


function access_control_apply(cf)
{
	if(cf.block_enable.checked == false)
		cf.hid_able_block_device.value = 0;
	else
		cf.hid_able_block_device.value = 1;
		
	if(cf.allow_or_block[0].checked == false)
		cf.hid_new_device_status.value = "Block";
	else
		cf.hid_new_device_status.value = "Allow";

	cf.submit_flag.value = "apply_acc_control";
}

function check_status()
{
	var cf = document.forms[0];
	var flag;
	flag = (!(cf.block_enable.checked));
	setDisabled(flag, cf.allow_or_block[0], cf.allow_or_block[1], cf.Allow, cf.Block, cf.all_checked);
	setDisabled(flag, cf.delete_allow_btn, cf.delete_block_btn, cf.allow_all, cf.block_all,cf.add_block_btn,cf.add_allow_btn);
	if(cf.block_enable.checked == false)
	{
		enable_block_device = 0;
		cf.Allow.className = "common_gray_bt";
		cf.Block.className = "common_gray_bt";
		for(i=0;i<access_control_device_num;i++)
			eval('document.getElementsByName("check_device'+i+'")[0]').disabled = true;

		cf.delete_allow_btn.className= "common_big_gray_bt";
		cf.add_allow_btn.className = "common_gray_bt";
		for(i=0;i<allowed_no_connect_num;i++)
			eval('document.getElementsByName("allow_not_connect'+i+'")[0]').disabled = true;

		cf.delete_block_btn.className= "common_big_gray_bt";
		cf.add_block_btn.className = "common_gray_bt";
		for(i=0;i<blocked_no_connect_num;i++)
			eval('document.getElementsByName("block_not_connect'+i+'")[0]').disabled = true;
	}
	else
	{
		enable_block_device = 1;
		cf.Allow.className = "common_bt";
		cf.Block.className = "common_bt";
		for(i=0;i<access_control_device_num;i++)
			eval('document.getElementsByName("check_device'+i+'")[0]').disabled = false;

		cf.delete_allow_btn.className= "common_big_bt";
		cf.add_allow_btn.className = "common_bt";
		for(i=0;i<allowed_no_connect_num;i++)
			eval('document.getElementsByName("allow_not_connect'+i+'")[0]').disabled = false;
		cf.delete_block_btn.className= "common_big_bt";
		cf.add_block_btn.className = "common_bt";

		for(i=0;i<blocked_no_connect_num;i++)
			eval('document.getElementsByName("block_not_connect'+i+'")[0]').disabled = false;
	}
}

function check_acc_add(cf)
{
	if(cf.acc_name.value == "")
	{
		alert("$device_name_null");
		return false;
	}
	if(cf.acc_mac.value.length==12 && cf.acc_mac.value.indexOf(":")==-1)
	{
		var mac=cf.acc_mac.value;
		cf.acc_mac.value=mac.substr(0,2)+":"+mac.substr(2,2)+":"+mac.substr(4,2)+":"+mac.substr(6,2)+":"+mac.substr(8,2)+":"+mac.substr(10,2);
	}
	else if ( cf.acc_mac.value.split("-").length == 6 )
	{
		var tmp_mac = cf.acc_mac.value.replace(/-/g,":");
		cf.acc_mac.value=tmp_mac;
	}
	if(maccheck(cf.acc_mac.value) == false)
		return false;
	if(acc_mac_num > 0)
	{
		for(i=0;i<acc_mac_num;i++)
		{
			var str = eval ( 'acc_mac' + i );
			if(str.toLowerCase() == cf.acc_mac.value.toLowerCase())
			{
				alert("$mac_dup");
				return false;
			}
		}
	}
	return true;
}
