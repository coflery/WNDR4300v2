#ifconfig eth0 up
# Recognize tag packet from CPU
ethreg 0x78=0x000001f0
ethreg 0x108=0xc03e0001


# Insert PVID 1 to WAN ports
ethreg 0x608=0x001b0001
ethreg 0x208=0x00390001
ethreg 0x408=0x00330001
ethreg 0x508=0x002b0001


# Insert PVID 2 to LAN port
ethreg 0x308=0x00010002

# Egress tag packet to CPU 
ethreg 0x104=0x00006204

# Egress untagged packet to LAN port
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

vconfig add eth0 1
vconfig add eth0 2
ifconfig eth0.1 up
ifconfig eth0.2 up

# Set mtu for eth0
ifconfig eth0 mtu 2000
ifconfig eth0.1 mtu 2000


