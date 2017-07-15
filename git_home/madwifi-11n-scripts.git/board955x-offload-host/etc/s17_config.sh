#register 0x0004 -> 0x07600000  (MAC0 RGMII, TX/RX delay enable)
#register 0x000c -> 0x07600000  (MAC6 RGMII, TX/RX delay enable)
#register 0x007c -> 0x0000007e  (MAC0 Mode, 1000M Full duplex with TX/RX flow control)
#register 0x0094 -> 0x0000007e  (MAC6 Mode, 1000M Full duplex with TX/RX flow control)
#register 0x0624 -> 0x007f7f7f  (PORT control)
#register 0x0010 -> 0x40000000  (if AR8327)

ethreg -i eth0 0x0004=0x07600000
ethreg -i eth0 0x000c=0x07600000
ethreg -i eth0 0x007c=0x0000007e
ethreg -i eth0 0x0094=0x0000007e
ethreg -i eth0 0x0624=0x007f7f7f
ethreg -i eth0 0x0010=0x40000000


#register 0x660 -> 0x0014001e
#register 0x420 -> 0x00010001
#--------------------------------
#register 0x66c -> 0x0014001d
#register 0x428 -> 0x00010001
#--------------------------------
#register 0x678 -> 0x0014001b
#register 0x430 -> 0x00010001
#--------------------------------
#register 0x684 -> 0x00140017
#register 0x4e8 -> 0x00010001
#--------------------------------
#register 0x690 -> 0x0014000f
#register 0x440 -> 0x00010001
#--------------------------------
#register 0x69c -> 0x00140040
#register 0x448 -> 0x00020001
#--------------------------------
#register 0x6a8 -> 0x00140020
#register 0x450 -> 0x00020001

ethreg -i eth0 0x660=0x0014001e
ethreg -i eth0 0x420=0x00010001
ethreg -i eth0 0x66c=0x0014001d
ethreg -i eth0 0x428=0x00010001
ethreg -i eth0 0x678=0x0014001b
ethreg -i eth0 0x430=0x00010001
ethreg -i eth0 0x684=0x00140017
ethreg -i eth0 0x4e8=0x00010001
ethreg -i eth0 0x690=0x0014000f
ethreg -i eth0 0x440=0x00010001
ethreg -i eth0 0x69c=0x00140040
ethreg -i eth0 0x448=0x00020001
ethreg -i eth0 0x6a8=0x00140020
ethreg -i eth0 0x450=0x00020001













