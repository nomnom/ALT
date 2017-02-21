#!/bin/bash

#1st arg: output dir
odir=/home/jergerml/AmbientLightTransfer/Software/cdata/plab
if [ -n "$1" ] ; then
 odir=`readlink -f "$1"`
fi
echo "capturing data to " $odir

mkdir $odir;

#2nd arg: delay on start in seconds
delay=10;
if [ -n "$2" ] ; then
 delay="$2"
fi

#3rd arg: shutterspeed (default 1 sec)
ss=1; if [ -n "$3" ] ; then ss=$3; fi

#4th arg: aperture size (default 2.8)
ap=2.8; if [ -n "$4" ] ; then ap=$4; fi

echo "output directory is $odir"
echo "aperture is $ap"
echo "shutterspeed is $ss"


imgtools=/home/jergerml/AmbientLightTransfer/Software/tools/imgtools

segsize=20
numsticks=6
numlamps=`echo "120 / $segsize * $numsticks * 3" | bc`
stick_config="--sticks -s $segsize -n $numsticks -d /dev/ttyACM0"
crop_area=2000x2000+1800+800
#crop_area=500x500+900+400
resize=500x500

# set default config
gphoto2 --camera "Canon EOS 5D Mark II" \
	--set-config autopoweroff=0 \
	--set-config iso=100 \
 	--set-config whitebalance=1 \
	--set-config aperture=$ap \
	--set-config shutterspeed=$ss \
	--set-config capturetarget=0

cd /home/jergerml/AmbientLightTransfer/Software/

#capture all-on image
echo "place white paper under sphere, press enter to continue"
./alt $stick_config --setlamps -u "1 1 1"

read


 gphoto2 --camera "Canon EOS 5D Mark II" \
        --capture-image-and-download --force-overwrite --filename $odir/all_raw.cr2

 # convert to 16 bit ppm and crop
 ( dcraw -4 $odir/all_raw.cr2 ; convert $odir/all_raw.ppm -crop $crop_area -resize $resize $odir/all.ppm ) &

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
 gphoto2 --camera "Canon EOS 5D Mark II" \
	  --capture-image-and-download --force-overwrite --filename $odir/img_raw_$l.cr2

 # process with dcraw, 16 bit halfsize image
 dcraw -4 $odir/img_raw_$l.cr2;

 #crop to sphere
 convert img_raw_$l.ppm -crop $crop_area img_$l.ppm 
 
 if [ $l -gt 0 ]
 then
   # subtract darkframe
   $imgtools -s -i $odir/img_$l.ppm -i $odir/img_0.ppm -o $odir/img_$l.ppm
   # resize
   convert img_$l.ppm -resize $resize img_$l.ppm
 fi
  
done


echo "calibrate completed on `date`" | mail -s "ALT notify" n0mfg64@googlemail.com
