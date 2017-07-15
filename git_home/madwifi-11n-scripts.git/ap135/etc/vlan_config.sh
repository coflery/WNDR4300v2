# Recognize tag packet from CPU
ethreg 0x78=0x000001f0
ethreg 0x108=0xc03e0001

# Insert PVID 1 to LAN ports
ethreg 0x608=0x00010001
ethreg 0x208=0x001d0001
ethreg 0x408=0x00170001
ethreg 0x508=0x000f0001

# Insert PVID 2 to WAN port 
ethreg 0x308=0x001b0002

# Egress tag packet to CPU and untagged packet to LAN port
ethreg 0x104=0x00006204

ethreg 0x204=0x00006104
ethreg 0x304=0x00006104 
ethreg 0x404=0x00006104
ethreg 0x504=0x00006104
ethreg 0x604=0x00006104

# Group port - 0,1,3,4,5 to VID 1 
ethreg 0x44=0x0000083B
ethreg 0x40=0x0001000a

# Group port - 0 and 2  to VID 2 
ethreg 0x44=0x00000805
ethreg 0x40=0x0002000a

brctl delif br0 eth0
brctl delif br0 eth1
ifconfig br0 down

vconfig add eth0 1
vconfig add eth0 2
ifconfig eth0.1 up
ifconfig eth0.2 up

ifconfig eth0.2 192.168.2.1

brctl addif br0 eth0.1
ifconfig br0 192.168.1.2

cfg -a AP_VLAN_MODE=1

echo 1 > /proc/sys/net/ipv4/ip_forward
