#!/bin/bash

#1st arg: output dir
odir=/home/jergerml/AmbientLightTransfer/Software/cdata/lamp1
if [ -n "$1" ] ; then
 odir="$1"
fi
echo "capturing data to " $odir

mkdir $odir;

#2nd arg: shutterspeed (default 1 sec)
ss=1; if [ -n "$2" ] ; then ss=$2; fi

#3rd arg: aperture size (default 2.8)
ap=2.8; if [ -n "$3" ] ; then ap=$3; fi

echo "output directory is $odir"
echo "aperture is $ap"
echo "shutterspeed is $ss"

crop_area=300x300+2700+2500
stick_config="--sticks -s120 -n2 -d /dev/ttyACM0"

# set default config
gphoto2 --camera "Canon EOS 5D Mark II" \
	--set-config autopoweroff=0 \
	--set-config iso=100 \
 	--set-config whitebalance=1 \
	--set-config aperture=$ap \
	--set-config shutterspeed=$ss

cd /home/jergerml/AmbientLightTransfer/Software/


numsteps=16

for l in `seq 0 1 $numsteps` 
do
 val=`python -c"print 1.0 / ($numsteps) * $l"` 
 echo "capturing image $l for green, brightness $val.."
 ./alt --debug --verbose $stick_config --setlamps -u "0 $val 0"
 gphoto2 --camera "Canon EOS 5D Mark II" \
	 --capture-image-and-download --force-overwrite --filename $odir/img_raw_$l.cr2
 # convert to 16 bit ppm and crop
 ( dcraw -4 $odir/img_raw_$l.cr2 ; convert $odir/img_raw_$l.ppm -crop $crop_area   $odir/img_$l.ppm ) &
done

for l in `seq 17 1 32` 
do
 val=`python -c"print 1.0 / ($numsteps) * ($l-$numsteps)"`
 echo "capturing image $l for red, brightness $val.."
 ./alt --debug --verbose $stick_config --setlamps -u "$val 0 0"
 gphoto2 --camera "Canon EOS 5D Mark II" \
         --capture-image-and-download --force-overwrite --filename $odir/img_raw_$l.cr2
 # convert to 16 bit ppm and crop
 ( dcraw -4 $odir/img_raw_$l.cr2 ; convert $odir/img_raw_$l.ppm -crop $crop_area   $odir/img_$l.ppm ) &
done

for l in `seq 33 1 48` 
do
 val=`python -c"print 1.0 / ($numsteps) * ($l-2*$numsteps)"`
 echo "capturing image $l for blue, brightness $val.."
 ./alt --debug --verbose $stick_config --setlamps -u "0 0 $val"
 gphoto2 --camera "Canon EOS 5D Mark II" \
         --capture-image-and-download --force-overwrite --filename $odir/img_raw_$l.cr2
 # convert to 16 bit ppm and crop
 ( dcraw -4 $odir/img_raw_$l.cr2 ; convert $odir/img_raw_$l.ppm -crop $crop_area   $odir/img_$l.ppm ) &
done

