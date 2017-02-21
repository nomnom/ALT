#!/bin/bash
# 
# apply .lamp file and capture result image
# 

secs=20
if [ "$2" -gt "0" ] ; then
 secs=$2
fi



imgtools=`pwd`/../Software/tools/imgtools



	# setup canon canon	
	aperture="5.6"
	shutterspeed="1"

	function configcam { #$1=0: RAM  $1=1: CF-Card
        gphoto2 --camera "Canon EOS 5D Mark II" \
                --set-config autopoweroff=0 \
                --set-config iso=100 \
                --set-config whitebalance=1 \
                --set-config aperture=$2 \
                --set-config shutterspeed=$3 \
                --set-config capturetarget=$1
        echo "configured canon with ap=$2 and ss=$3"
    }
        
	function capture {
		gphoto2 --camera "Canon EOS 5D Mark II" --capture-image-and-download --force-overwrite --filename $1
	}
    
    function download {
		gphoto2 --camera "Canon EOS 5D Mark II" --get-file $1 --force-overwrite --filename $2
	}
		
	function clearcam {
		gphoto2 --camera "Canon EOS 5D Mark II" --folder /store_00010001/DCIM/100EOS5D --delete-all-files 
	}



	#
	# start !
	#
		
	echo "starting in.."
	for i in `seq $secs -1 1` ; do echo -n "$i "; sleep 1; done
	echo "GO!"





configcam 0 $aperture $shutterspeed
ldir=`readlink -f "$1"`

	cd ../
	
if [ 0 -gt 1 ] ; then
	#darkframe
	./alt --setlamps -u "0 0 0"  --sticks -n8 -s120
	capture ${ldir}/darkframe.cr2
	
	#all-on image
	./alt --setlamps -u "1 1 1"  --sticks -n8 -s120
	capture ${ldir}/white.cr2	

	m=`echo 1.0 / 255.0 | bc -l`
	./alt --setlamps -u "1 $m $m"  --sticks -n8 -s120
	capture ${ldir}/r1.cr2	
	./alt --setlamps -u "$m 1 $m"  --sticks -n8 -s120
	capture ${ldir}/g1.cr2	
	./alt --setlamps -u "$m $m 1"  --sticks -n8 -s120
	capture ${ldir}/b1.cr2	

	m=0
	./alt --setlamps -u "1 $m $m"  --sticks -n8 -s120
	capture ${ldir}/r0.cr2	
	./alt --setlamps -u "$m 1 $m"  --sticks -n8 -s120
	capture ${ldir}/g0.cr2	
	./alt --setlamps -u "$m $m 1"  --sticks -n8 -s120
	capture ${ldir}/b0.cr2	
	
	m=`echo 5.0 / 255.0 | bc -l`
	./alt --setlamps -u "1 $m $m"  --sticks -n8 -s120
	capture ${ldir}/r5.cr2	
	./alt --setlamps -u "$m 1 $m"  --sticks -n8 -s120
	capture ${ldir}/g5.cr2	
	./alt --setlamps -u "$m $m 1"  --sticks -n8 -s120
	capture ${ldir}/b5.cr2	
fi;
cd $ldir


for d in white darkframe r1 g1 b1 r0 g0 b0 r5 g5 b5; do
  dcraw -h -4 $d.cr2
done

for d in white r1 g1 b1 r0 g0 b0 r5 g5 b5; do
	imgtools -s -i $d.ppm -i darkframe.ppm -o $d.ppm
done


echo "done";

