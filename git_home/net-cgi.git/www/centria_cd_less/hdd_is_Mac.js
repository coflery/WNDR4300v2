function detectOS() 
{   // detect the system type of user computer
    var sUserAgent = navigator.userAgent;  
    var isWin = (navigator.platform == "Win32") || (navigator.platform == "Windows");  
    var isMac = (navigator.platform == "Mac68K") || (navigator.platform == "MacPPC") || (navigator.platform == "Macintosh") || (navigator.platform == "MacIntel");  
    if (isMac) return "Mac";  
    var isUnix = (navigator.platform == "X11") && !isWin && !isMac;  
    if (isUnix) return "Unix";  
    var isLinux = (String(navigator.platform).indexOf("Linux") > -1);  
    if (isLinux) return "Linux";  
    if (isWin) {  
        var isWin2K = sUserAgent.indexOf("Windows NT 5.0") > -1 || sUserAgent.indexOf("Windows 2000") > -1;  
        if (isWin2K) return "Win2000";  
        var isWinXP = sUserAgent.indexOf("Windows NT 5.1") > -1 || sUserAgent.indexOf("Windows XP") > -1;  
        if (isWinXP) return "WinXP";  
        var isWin2003 = sUserAgent.indexOf("Windows NT 5.2") > -1 || sUserAgent.indexOf("Windows 2003") > -1;  
        if (isWin2003) return "Win2003";  
        var isWin2003 = sUserAgent.indexOf("Windows NT 6.0") > -1 || sUserAgent.indexOf("Windows Vista") > -1;  
        if (isWin2003) return "WinVista";  
        var isWin2003 = sUserAgent.indexOf("Windows NT 6.1") > -1 || sUserAgent.indexOf("Windows 7") > -1;  
        if (isWin2003) return "Win7";  
    }  
    return "None";  
} 

function dl_wait()
{
	document.write('<div class="wizard_content_div">');
	document.write('<div class="wizard_words_div">');
	document.write('<div style="height:40px; width:100%; margin:15% 0 0 0;" align="center">');
	document.write('<h1>');
	document.write('<p align=center style="color:#000000; font-family:Verdana; font-size:14px; font-weight: bold;"> This will take half minute, please wait?</p>');
	document.write('</h1>');
	document.write('</div>');
	document.write('<div class="waiting_img_div" align="center">');
	document.write('<img src="../image/wait30.gif" />');
	document.write('</div>');
	document.write('</div>');
	document.write('</div>');
}
