var sata_vol_name= new Array( );
var sata_part_name=new Array( );
var sata_vol_num =0;
function sata_vol()
{
		var diff_num = 0;
	//alert("enter sata patiotion info save");
	
	for( i=0; i<disk_num; i++){
		 
		var volname = eval('dev' + i);
			volname_each_info = volname.split('*');
			
			if( volname_each_info[0].indexOf(sata_exist) != -1 && sata_exist.length >0)
			{	
				sata_part_name[sata_vol_num]=volname_each_info[0];
				//var volname_str = eval('dev_vol' + i);
				//volname_str = volname_str.replace(/&lt;/g,"<").replace(/&gt;/g,">").replace(/&#40;/g,"(").replace(/&#41;/g,")").replace(/&#34;/g,'\"').replace(/&#39;/g,"'").replace(/&#35;/g,"#").replace(/&#38;/g,"&");
				//volname_str = volname_str.split(' ');
				//sata_vol_name[sata_vol_num]=volname_str[0];
				sata_vol_name[sata_vol_num]=volname_each_info[4].replace(/&lt;/g,"<").replace(/&gt;/g,">").replace(/&#40;/g,"(").replace(/&#41;/g,")").replace(/&#34;/g,'\"').replace(/&#39;/g,"'").replace(/&#35;/g,"#").replace(/&#38;/g,"&");
				sata_vol_num =sata_vol_num+1;
			}
	}
}

var have_reserved = 1;
function sata_reserved()
{
	if(sata_vol_num != 0)
	{
		for(i=0; i<folder_number; i++) 
		{
			var sata_folder_name = eval('usb_sharefolder' + i);
			var sata_folder_info = sata_folder_name.split('*');
			
			if(sata_folder_info[1].indexOf(sata_part_name[0]) != -1 )
				have_reserved = 0;		
		}
	}
}