#!/bin/bash

if [ -z "$1" ];then 
 exp=1000
else
 exp="$1"
fi


viddev="$2"
if [ -z "$viddev" ];then 
 viddev=`ls /dev/ | grep video | head`
fi
if [ -z "$viddev" ];then 
 viddev=/dev/video0
 echo  "Error: no video device found in /dev/"
else 
 echo "Using video device $viddev" 
fi

if [ -n "`lsusb | grep "Live! Cam Chat HD"`" ]
then
  echo  "Found Creative Live! Cam Chat HD on USB bus"
  lsusb | grep "Live! Cam Chat HD" | cut -d' ' -f-6
else  
  echo  "Error: Creative Live! Cam Chat HD not found!"
fi




uvcdynctrl -d $viddev -s "Exposure (Absolute)" $exp

guvcview -l creative_default.gpfl -d $viddev -f yuyv  -s 320x240 --hwd_acel=1 


