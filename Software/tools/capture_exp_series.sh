#!/bin/bash

#
# note: works only with guvcview 1.6.1
#

vdev=/dev/video0
odir=/home/jergerml/tmp
if [ -n "$1" ] ; then
 odir="$1"
fi

# activate manual exposure
uvcdynctrl -d $vdev -s "Exposure, Auto" 1

for exp in 500 300 200 100 75 50 25 10
#for exp in 300 200 150 100 75 50 30 20 10
#for exp in 1250 1000 750 500 300 200 100 75 50 25 10
do
 uvcdynctrl -d $vdev -s "Exposure (Absolute)" $exp
 sleep 1
 for rep in 0 1 2 
 do
  file=img_${exp}-${rep}

  # take picture 
  guvcview --verbose --no_display --profile creative_default.gpfl --device=$vdev \
          --cap_time=1 --exit_on_close --skip=5 --npics=2 --size=1280x720 --image=$odir/$file.jpg

  # convert to ppm
  convert $odir/$file.jpg $odir/$file.ppm
  rm $odir/$file.jpg

  # add entry to .hdrgen
  echo $file.ppm `echo 1 / $exp | bc -l`1 1 0 >> $odir/exps_all.hdrgen
 
 done

 # create average image
 convert -adjoin ${odir}/img_${exp}-0.ppm ${odir}/img_${exp}-1.ppm ${odir}/img_${exp}-2.ppm -average ${odir}/img_${exp}-avg.ppm
 
 # add entry to .hdrgen
 echo img_${exp}-avg.ppm `echo 1 / $exp | bc -l`1 1 0 >> $odir/exps_avg.hdrgen
done

# recover camera response
cd ${odir}
pfsinhdrgen exps_avg.hdrgen | pfshdrcalibrate -Y -x -s response_lum_avg.m > /dev/null
pfsinhdrgen exps_avg.hdrgen | pfshdrcalibrate -x -s response_rgb_avg.m > /dev/null

# split up in r/g/b
head -n 263 response_rgb_avg.m | tail -n 263 > response_r.m
head -n 526 response_rgb_avg.m | tail -n 263 > response_g.m
head -n 789 response_rgb_avg.m | tail -n 263 > response_b.m


# plot data in nonlog space
echo "plot 'response_lum_avg.m' using 3:2 w l" | gnuplot -p -
echo "plot 'response_r.m' using 3:2 w l, 'response_g.m' using 3:2 w l, 'response_b.m' using 3:2 w l" | gnuplot -p -
