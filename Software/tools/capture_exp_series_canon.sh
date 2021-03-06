#!/bin/bash

odir=/home/jergerml/tmp
if [ -n "$1" ] ; then
 odir="$1"
fi

sleep 2


# set default config

gphoto2 --camera "Canon EOS 5D Mark II" \
	--set-config autopoweroff=0 \
	--set-config iso=100 \
 	--set-config whitebalance=1 \
	--set-config aperture=0 \
	--set-config shutterspeed=1


echo -n "" > $odir/exps_all.hdrgen

i=0

for s in 6 4 3.2 2 1.6 1 0.8 0.5 0.3 1/4 1/5 1/8 1/6 1/15 1/20 1/30 1/50   
#for s in  1/15 1/50  1/100 1/200 1/400 
do
 echo $s

 gphoto2 --camera "Canon EOS 5D Mark II" \
	  --set-config shutterspeed=$s \
	  --capture-image-and-download --force-overwrite --filename $odir/img_$i.cr2
 
 # convert from raw to ppm
 dcraw -4 $odir/img_$i.cr2 & 

 # add entry to .hdrgen
 echo img_$i.ppm $s 2.8 100 0 >> $odir/exps_all.hdrgen  
   
 i=`echo 1 + $i | bc`
  
done

cd $odir
# pfsinhdrgen exps_all.hdrgen | pfshdrcalibrate  -b 16 -Y -x -s response_lum.m > /dev/null  

# recover reponse curves
pfsinhdrgen exps_all.hdrgen | pfshdrcalibrate  -b 16 -x -s response_rgb.m > /dev/null  

# split into channels
head -n 65542 response_rgb.m > response_r.m && head -n `echo "65543 * 2" | bc` response_rgb.m | tail -n 65542 > response_g.m && head -n `echo "65543 * 3" | bc` response_rgb.m | tail -n 65542 > response_b.m

# create array data
#cat response_r.m | sed s/\s+/\ /g | cut -d' ' -f 3- > reponse.c

