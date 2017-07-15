# Readme file for setting the WPS in AP and STA

1. Building hostapd and supplicant
    - make BOARD_TYPE=x86-host-small  athr-hostapd athr-wpa_supplicant    

2. Starting hostapd & Supplicant 
  
- For AP side
    - copy hostapd and hostapd_cli /usr/local/bin  
    - Load the tartget fw.rom.bin, before calling the scripts below
    - Set the ssid, wps_state=1 , WPS parameters & security params given in sample WSC.ap.conf config file 
    - use the correct config file in setup.ap_wps file
    - run the script setup.ap_wps

- For Station side
    - copy wpa_supplicant and wpa_cli /usr/local/bin  
    - Load the target fw.rom.bin , before calling the scripts below 
    - Set the wps params as mentioned in sample WSC.sta.conf config file 
    - use the correct config file in setup.sta_wps file
    - run the script setup.sta_wps

3. WPS Push method 
- For AP side
    - hostapd_cli wps_pbc
- For Sta side 
    - wpa_cli wps_pbc any


4. WPS Pin method 
- For Sta side 
    - wpa_cli wps_pin any
    - above command will return the pin, enter sta pin in AP side as below
- For AP side
    - hostapd_cli wps_pin any <pin>

- Set the ipaddress for AP and STA after both the ap and sta mode successfully connected via security mode

