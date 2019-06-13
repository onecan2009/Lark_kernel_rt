#!/bin/bash
set -e
#echo "config"
#make firefly-rk3288_defconfig
DATESTAMP=`date "+%Y_%m_%d_%H_%M_%S"`
echo "make img..."
make firefly-rk3288.img

echo "mkbootimg..."
mkbootimg --kernel arch/arm/boot/zImage --ramdisk initrd.img -o boot.img

echo "cp.."
cp boot.img  /samba_dir/ubuntu_share/temp_test
cp resource.img  /samba_dir/ubuntu_share/temp_test

echo "over.."
