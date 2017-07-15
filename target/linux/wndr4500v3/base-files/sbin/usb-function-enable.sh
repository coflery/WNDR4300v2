#!/bin/sh 

insmod /lib/ufsd/ufsd.ko
# Insmod readyshare printer modules, and it must be insmod after "br0" up
insmod /lib/modules/2.6.31/GPL_NetUSB.ko
insmod /lib/modules/2.6.31/NetUSB.ko

# Support TUN interface
mkdir -p /dev/net
[ -c /dev/net/tun ] || mknod /dev/net/tun c 10 200
chmod 0700 /dev/net/tun

# Disable Readyshare Cloud 
#/etc/init.d/remote.sh start
#/etc/init.d/leafp2p.sh start
#/etc/init.d/brokerd.sh start

/etc/init.d/samba start

/usr/sbin/net-disk

#power on USB socket
echo 1 > /proc/simple_config/usb5v
echo 1 > /proc/simple_config/usb5v_0
echo 1 > /proc/simple_config/usb5v_1

if [ ! -f /tmp/failover_init_done ]; then
        /etc/init.d/failover.init boot
fi

/etc/init.d/afpd start

/etc/init.d/avahi-daemon start

# add jffs2 filesystem support
[ -f /dev/mtdblock13 ] && mount -t jffs2 /dev/mtdblock13 /jffs/

