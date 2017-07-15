# Recognize tag packet from CPU
ethreg -i eth0 0x620=0x000004f0
ethreg -i eth0 0x660=0x0014017e
ethreg -i eth0 0x66c=0x0014017d
ethreg -i eth0 0x678=0x0014017b
ethreg -i eth0 0x684=0x00140177
ethreg -i eth0 0x690=0x0014016f
ethreg -i eth0 0x69c=0x0014015f

# Insert PVID 1 to LAN ports
ethreg -i eth0 0x420=0x00010001
ethreg -i eth0 0x430=0x00010001
ethreg -i eth0 0x438=0x00010001
ethreg -i eth0 0x440=0x00010001
ethreg -i eth0 0x448=0x00010001

# Insert PVID 2 to Target port 
ethreg -i eth0 0x428=0x00020001

# Egress tag packet to CPU and untagged packet to LAN port
ethreg -i eth0 0x424=0x00002040
ethreg -i eth0 0x42c=0x00001040
ethreg -i eth0 0x434=0x00001040
ethreg -i eth0 0x43c=0x00001040
ethreg -i eth0 0x444=0x00001040
ethreg -i eth0 0x44c=0x00001040


vconfig add eth1 2
ifconfig eth1.1 up
ifconfig eth1.2 up

#cfg -a AP_VLAN_MODE=1

echo 1 > /proc/sys/net/ipv4/ip_forward
