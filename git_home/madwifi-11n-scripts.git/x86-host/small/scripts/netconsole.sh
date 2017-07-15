# script to stop wpa supplicant and network manager 
# and set up sending console messages over ethernet to another machine.
sudo service network-manager stop
sudo killall wpa_supplicant
# Here you need to specify the target ip/tareget mac address to send console messages to
# another machine.
# the semantics are 
# sudo modrobe netconsole="@target ip address/target interface, remote port@remote ip/remote mac address"
# on the remote host you need to ru the following command
# nc -u -p remote port (9353) -s <remote ip>    <target ip> 6665 
# remmote refers to the machine where you intend to capture console log messages.
# target refers to the machine that has the linux kerenl/driver running.
sudo modprobe netconsole netconsole="@10.234.18.243/eth0,9353@10.234.17.164/00:26:bb:51:2c:56"
sudo dmesg -n7
