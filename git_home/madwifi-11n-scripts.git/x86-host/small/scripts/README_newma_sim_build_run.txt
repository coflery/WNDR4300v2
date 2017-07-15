This file contains the instructions to build and execute the qca main ofload stack on the simulator 
for both AP and STA modes.  Use two x86 based laptops/desktops with ubunti 11.0 for building and 
testing your software. Use one of them as AP and the other as STA.
It would be a good idea to have these systems in a private network to enable easy debugging. 

1) Source code from perforce
============================
You need the software  from the //depot/sw/qca_main/... branch. which contains both host and target. 

Check out the  //depot/sw/qca_main/... tree from p4 using the following sample spec
View:
        //depot/sw/qca_main/... //<your clientspec>/qca_main/...
        //depot/sw//qca_main/drivers/adf/... //<your clientspec>/qca_main/drivers/wlan_modules/adf/...
        //depot/sw/qca_main/drivers/asf/... //<your clientspec>/qca_main/drivers/wlan_modules/asf/...


2) Build host software on the x86 machine
=======================================
  Go to the ~/qca_main/build directory and invoke
   >make driver_build BOARD_TYPE=x86-host-small BUS_TYPE=sim

3) Build target software (firmware and simulator)
==============================================
Switch to your ~/qca_main/perf_pwr_offload/drivers directory.
 a) run the quickbuild.sh in the same directory.
    NOTE:you may have to edit the following line in the script to set your linux source path
    before running the script.
    export ATH_LINUXPATH

 b) run the following command in the same directory.
    >make clobber; make

4) Bring up  one machine in AP mode
======================================
 a) On the x86 machine open two windows( this is just for your convenience )
 b) On the first window(host) switch to the ~/qca_main/scripts directory and invoke
      > sudo ./rc.wlan_sim up
 c) On the second windo(wtarget) switch to the ~/qca_main/perf_pwr_offload/drivers directory.
    run the quickbuild.sh .
     > source quickbuild.sh
 d) switch to target sub directory and start the target simulator.
    > cd target
    > sudo -E .output/AR9888/sim.1/image/sw.rom.out -h -a firmware -he wifi-sim0 -m 00:11:22:33:44:55  --port xxxxx
 e) On the first window (host), invoke the following script which will
    create ap wlan interface (vdev) and bring up AP.
   > sudo ./setup.ap_sim

This will bring up the wlan interface on the AP and start beaconing. 
optionally you can run sudo tcpdump -x -i ethx to see the beacon frames


5) Bring up sedond one in STA mode
====================================
 a) On the x86 machine open two windows( this is just for your convenience )
 b) On the first window(host) switch to the ~/qca_main/scripts directory and invoke
    > sudo ./rc.wlan_sim up
 c) On the second windo(wtarget) switch to the ~/qca_main/perf_pwr_offload/drivers directory.
    run the quickbuild.sh .
    > source quickbuild.sh 
 d) switch to target sub directory and start the target simulator.
    > cd target
    > sudo -E .output/AR9888/sim.1/image/sw.rom.out -h -a firmware -he wifi-sim0 -m 00:33:44:55:66:77 --port xxxxx
 e) On the first window (host), invoke the following script which will
    create ap wlan interface (vdev) and bring up STA.
    > sudo ./setup.sta_sim -s 0-open-wlan0 [or use whatever SSID is being advertised by your AP from step 4]
  This will bring up the wlan interface on the STA and kick off scanning and association.
  you can run iwconfig to check the state of the station interface.
  you can now ping the AP from STA. check the ip address of the AP and STAs to run the ping test.
  optionally you can run sudo tcpdump -x -i ethx to examine the management/data frames


6) To Bring down the drivers (AP or STA)
=============================================
On the first window(host) invoke
> ./setup.del_sim
On the second window(targets sim) invoke
> CTL+C or quit
>On the first window(host) invoke
> sudo ./rc.wlan_sim down  (This will unload the driver)

If you successfully tested the Unified UMAC following the steps above give yourself a pat on your shoulder and take
the rest of the day off:-)


