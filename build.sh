#!/bin/bash

#echo "config"
#make firefly-rk3288_defconfig
DATESTAMP=`date "+%Y_%m_%d_%H_%M_%S"`
echo "make img..."
make firefly-rk3288.img

echo "mkbootimg..."
mkbootimg --kernel arch/arm/boot/zImage --ramdisk initrd.img -o boot.img

echo "cp.."
cp boot.img  /mnt/hgfs/share/kernel_3.10.79/boot_${DATESTAMP}.img
cp resource.img  /mnt/hgfs/share/kernel_3.10.79/resource_${DATESTAMP}.img

echo "over.."
