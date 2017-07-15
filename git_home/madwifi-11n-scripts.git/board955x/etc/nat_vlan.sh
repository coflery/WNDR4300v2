# iptables config
iptables --flush
iptables --table nat --flush
iptables --delete-chain
iptables --table nat --delete-chain
iptables -A FORWARD -j ACCEPT -i eth0.1 -o eth0.2  -m state --state NEW
iptables -A FORWARD -m state --state ESTABLISHED,RELATED  -j ACCEPT
iptables -A POSTROUTING -t nat -o eth0.2 -j MASQUERADE 
echo 1 >  /proc/sys/net/ipv4/ip_forward

# WAN - LAN/WLAN access
iptables -I PREROUTING -t nat -i eth0.2 -p tcp --dport 1000:20000 -j DNAT --to 192.168.1.100 -m state --state NEW
iptables -I FORWARD -i eth0.2 -p tcp --dport 1000:20000 -d 192.168.1.100 -j ACCEPT
