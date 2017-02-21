#!/bin/bash

#1st arg: output dir

#1st arg: delay on start in seconds
delay=10;
if [ -n "$1" ] ; then
 delay="$1"
fi

#3rd arg: shutterspeed (default 1 sec)
ss=1; if [ -n "$2" ] ; then ss=$2; fi

#4th arg: aperture size (default 2.8)
ap=3.5; if [ -n "$3" ] ; then ap=$3; fi

echo "output directory is $odir"
echo "aperture is $ap"
echo "shutterspeed is $ss"

segsize=10
numsticks=8
numlamps=`echo "120 / $segsize * $numsticks * 3" | bc`
stick_config="--sticks -s $segsize -n $numsticks -d /dev/ttyACM0"
echo "sticks config is " $stick_config
echo "numlamps is $numlamps"

# set default config
gphoto2 --camera "Canon EOS 5D Mark II" \
	--set-config autopoweroff=0 \
	--set-config iso=100 \
 	--set-config whitebalance=1 \
	--set-config aperture=$ap \
	--set-config shutterspeed=$ss \
	--set-config capturetarget=1

cd /home/jergerml/AmbientLightTransfer/Software/

#capture all-on image
echo "place white paper under sphere, press enter to continue"
./alt $stick_config --setlamps -u "0.5 0.5 0.5"

read


 gphoto2 --camera "Canon EOS 5D Mark II" --capture-image 

echo "remove white paper and press enter to start capture routine"
read

echo "starting in.."
for i in `seq $delay -1 1` ; do echo -n "$i "; sleep 1; done
echo "GO!"
echo "calibrate started on `date`" | mail -s "ALT notify" n0mfg64@googlemail.com


trap "exit" INT
for l in `seq 0 1 $numlamps` 
do
 
 if [ $l -eq 0 ]
 then
   #darkframe
   ./alt $stick_config --setlamps -u "0 0 0"
 else
   # turn on lamp
   ./alt $stick_config --setlamps -s "`echo $l - 1 | bc` 1.0"
 fi

 # take picture
 gphoto2 --camera "Canon EOS 5D Mark II"  --capture-image

done


echo "calibrate completed on `date`" | mail -s "ALT notify" n0mfg64@googlemail.com




