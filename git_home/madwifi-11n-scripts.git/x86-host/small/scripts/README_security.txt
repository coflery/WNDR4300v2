# Readme file for setting the security modes in AP and STA

Open/Shared WEP Security
-------------------

- For AP side
    - Load the tartget fw.rom.bin, before calling the scripts below
    - use the setup.ap_wep file to configure 
    - for setting the shared authentication add the following command to setup.ap_wep
         iwpriv wlan0 authmode 2

-For Station side
    - Load the tartget fw.rom.bin, before calling the scripts below
    - use the setup.sta_wep file to configure 
    - for setting the shared authentication add the following command to setup.sta_wep
         iwpriv wlan0 authmode 2
       
   

WPA based security
------------------

- Building hostapd and supplicant
    - make BOARD_TYPE=x86-host-small  athr-hostapd athr-wpa_supplicant    
  
- For AP side  
    - Load the tartget fw.rom.bin, before calling the scripts below
    - Set the ssid, passphrase and other security params given in sample hostapd config file (PSK.ap_bss_TKIP or PSK.ap_bss_CCMP)
    - use the correct config file in setup.ap_wpa_security file
    - run the script setup.ap_wpa_security

- For Station side
    - Load the target fw.rom.bin i, before calling the scripts below 
    - Set the ssid, passphrase and other security params given in sample supplicant config file (PSK.sta_TKIP or PSK.sta_CCMP)
    - use the correct config file in setup.sta_wpa_security file
    - run the script setup.sta_wpa_security

- Set the ipaddress for AP and STA after both the ap and sta mode successfully connected via security mode



WAPI based security
-------------------

- Building WAPI Authenticator 
    - make BOARD_TYPE=x86-host-small wapi_build    
  
- For AP side  
    - Load the tartget fw.rom.bin, before calling the scripts below
    - Set the ssid, passphrase and other WAPI security params given in sample wapi config file (WAPI.PSK.ap) 
    - use the correct config file in setup.ap_wapi_security file 
    - run the script setup.ap_wapi_security

- For Station side
    - There is no WAPI support for Newma linux client . Currently WAPI is validated with Newma WAPI authenticator 
and Toba WAPI client. To setup Toba WAPI client follow Toba WAPI client setup procedures.
