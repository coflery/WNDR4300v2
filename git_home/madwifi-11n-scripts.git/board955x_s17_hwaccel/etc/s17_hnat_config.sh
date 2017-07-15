brctl delif br0 eth1
ifconfig br0 0.0.0.0 up
ifconfig eth0 0.0.0.0 up 
brctl addif br0 eth0
vconfig add eth1 1
vconfig add eth1 2 
ifconfig eth1.1 192.168.1.2 up
ifconfig eth1.2 10.10.2.1 up
ifconfig eth1.2 netmask 255.255.255.0
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 1 > /proc/sys/net/ipv4/conf/br0/arp_ignore

# iptables config
iptables --flush
iptables --table nat --flush
iptables --delete-chain
iptables --table nat --delete-chain
iptables -A FORWARD -j ACCEPT -i eth1.1 -o eth1.2  -m state --state NEW
iptables -A FORWARD -m state --state ESTABLISHED,RELATED  -j ACCEPT
iptables -A POSTROUTING -t nat -o eth1.2 -j MASQUERADE

# WAN - LAN/WLAN access
iptables -I PREROUTING -t nat -i eth1.2 -p tcp --dport 1000:20000 -j DNAT --to 192.168.1.100 -m state --state NEW
iptables -I FORWARD -i eth1.2 -p tcp --dport 1000:20000 -d 192.168.1.100 -j ACCEPT
