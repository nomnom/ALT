#!/bin/bash

segsize=10
numsticks=8
numlamps=`echo "120 / $segsize * $numsticks * 3" | bc`
numlampsplusone=`echo "$numlamps + 1" | bc`
stick_config="--sticks -s $segsize -n $numsticks -d /dev/ttyACM0"
#crop_area=2000x2000+1800+800
crop_area=800x800+1000+550
resize=400x400
imgtools=/home/jergerml/AmbientLightTransfer/Software/tools/imgtools

#1st arg: output dir
odir=/home/jergerml/AmbientLightTransfer/Software/cdata/plab
if [ -n "$1" ] ; then
 odir=`readlink -f "$1"`
fi
echo "directory is " $odir

cd $odir

echo "downloading images"
gphoto2 --camera "Canon EOS 5D Mark II" --get-file 1 --filename $odir/all_raw.cr2
exiftool -CameraOrientation=0 -n all_raw.cr2
dcraw -4 -h all_raw.cr2
convert all_raw.ppm -crop $crop_area all.ppm
convert all.ppm -resize $resize all.ppm

for l in `seq 1 1 $numlampsplusone`
do
echo
  gphoto2 --camera "Canon EOS 5D Mark II" --get-file `echo "($l)*2+1" | bc` --filename $odir/img_raw_`echo "$l - 1" | bc`.cr2
done

echo "converting images"
for l in `seq 0 1 $numlamps`
do
 echo $l;

 # remove possible rotation tag
 exiftool -CameraOrientation=0 -n img_raw_$l.cr2
 # process with dcraw
 dcraw -4 -h $odir/img_raw_$l.cr2;

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

convert img_0.ppm -resize $resize img_0.ppm
