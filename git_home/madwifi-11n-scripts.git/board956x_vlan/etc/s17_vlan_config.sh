ifconfig eth0 up

# Recognize tag packet from CPU
ethreg -i eth0 0x620=0x000004f0
ethreg -i eth0 0x660=0x0014017e
ethreg -i eth0 0x66c=0x0014017d
ethreg -i eth0 0x678=0x0014017b
ethreg -i eth0 0x684=0x00140177
ethreg -i eth0 0x690=0x0014016f
ethreg -i eth0 0x69c=0x0014015f

# Insert PVID 1 to LAN ports
#port0
ethreg -i eth0 0x420=0x00010001
#port2,phy1
ethreg -i eth0 0x430=0x00010001 
#port3,phy3
ethreg -i eth0 0x438=0x00010001
#port4,phy3
ethreg -i eth0 0x440=0x00010001
#port5,phy4
ethreg -i eth0 0x448=0x00010001 

# Insert PVID 2 to WAN port (Phy0)
ethreg -i eth0 0x428=0x00020001

# Egress tag packet to CPU and untagged packet to LAN port
ethreg -i eth0 0x424=0x00002040
ethreg -i eth0 0x42c=0x00001040
ethreg -i eth0 0x434=0x00001040
ethreg -i eth0 0x43c=0x00001040
ethreg -i eth0 0x444=0x00001040
ethreg -i eth0 0x44c=0x00001040

# Group port - 0,,2,3,4,5 to VID 1 
ethreg -i eth0 0x610=0x001b55e0
ethreg -i eth0 0x614=0x80010002

# Group port - 0 and 5  to VID 2
ethreg -i eth0 0x610=0x0019ff60
ethreg -i eth0 0x614=0x80020002

vconfig add eth0 1
vconfig add eth0 2

ip link set eth0.1 name LAN
ip link set eth0.2 name WAN

cfg -a AP_VLAN_MODE=1


