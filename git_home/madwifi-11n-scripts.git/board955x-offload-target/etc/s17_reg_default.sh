#
#Enable forwarding to all ports and 
#disable learning (needed for VLAN offload where same MAC addresses can come from different ports)

ethreg -i eth0 0x660=0x0004017e
ethreg -i eth0 0x66c=0x0004017d
ethreg -i eth0 0x678=0x0004017b
ethreg -i eth0 0x684=0x00040177
ethreg -i eth0 0x690=0x0004016f
ethreg -i eth0 0x69c=0x0004015f
#In AP135 , do this for GMAC6 port (WAN) also
ethreg -i eth0 0x6a8=0x0004013f

# Insert PVID 1 to LAN ports
ethreg -i eth0 0x420=0x00010001
ethreg -i eth0 0x428=0x00010001
ethreg -i eth0 0x430=0x00010001
ethreg -i eth0 0x438=0x00010001
ethreg -i eth0 0x440=0x00010001
ethreg -i eth0 0x448=0x00010001
ethreg -i eth0 0x450=0x00010001

# Egress unmodified packets to all ports
ethreg -i eth0 0x424=0x00003040
ethreg -i eth0 0x42c=0x00003040
ethreg -i eth0 0x434=0x00003040
ethreg -i eth0 0x43c=0x00003040
ethreg -i eth0 0x444=0x00003040
ethreg -i eth0 0x44c=0x00003040
ethreg -i eth0 0x454=0x00003040

