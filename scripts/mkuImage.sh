#!/bin/sh -x

#
# $1 == mkuImage tools path
# $2 == kernel tree path
# $3 == Image path
MKIMAGE=$1/mkimage
VMLINUX=$2/linux-2.6.15/vmlinux
VMLINUXBIN=$2/vmlinux
LDADDR=0x80002000

ENTRY=`readelf -a ${VMLINUX}|grep "Entry"|cut -d":" -f 2`

$1/mips-linux-uclibc-objcopy -O binary --remove-section=.reginfo --remove-section=.mdebug --remove-section=.comment --remove-section=.note --remove-section=.pdr --remove-section=.options --remove-section=.MIPS.options $VMLINUX $3/vmlinux.bin

cp $3/vmlinux.bin $3/wndr3700u-vmlinux.bin

gzip -f $3/vmlinux.bin

$1/lzma e $3/wndr3700u-vmlinux.bin $3/vmlinux.bin.lzma

${MKIMAGE} -A mips -O linux -T kernel -C lzma \
        -a ${LDADDR} -e ${ENTRY} -n "Linux Kernel Image"    \
                -d $3/vmlinux.bin.lzma $3/vmlinux.lzma.uImage

${MKIMAGE} -A mips -O linux -T kernel -C gzip \
        -a ${LDADDR} -e ${ENTRY} -n "Linux Kernel Image"    \
                -d $3/vmlinux.bin.gz $3/vmlinux.gz.uImage
