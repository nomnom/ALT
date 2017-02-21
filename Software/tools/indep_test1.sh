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
	aperture="2.8"
	shutterspeed="2"

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
	echo "indep test started on `date`" | mail -s "ALT started" n0mfg64@googlemail.com





configcam 0 $aperture $shutterspeed
ldir=`readlink -f "$1"`

	cd ../
	

	#all-on image
	./alt --setlamps -u "1 1 1"  --sticks -n8 -s120
	capture ${ldir}/all.cr2	

    

	
		
	#darkframe
	./alt --setlamps -u "0 0 0"  --sticks -n8 -s120
	capture ${ldir}/darkframe.cr2
	
	
        # seg 1
        ./alt --setlamps -s "113 1" -s "125 1" -s "137 1"  --sticks -n8 -s10 
	capture ${ldir}/s1_white.cr2
	
        # seg 2
        ./alt --setlamps -s "112 1" -s "124 1" -s "136 1"  --sticks -n8 -s10 
	capture ${ldir}/s2_white.cr2

        #both	
        ./alt --setlamps  -s "112 1" -s "124 1" -s "136 1" -s "113 1" -s "125 1" -s "137 1"  --sticks -n8 -s10 
	capture ${ldir}/white.cr2
		
       #single colors
        # seg 1 : upper
        ./alt --setlamps -s "113 1"  --sticks -n8 -s10
        capture ${ldir}/s1_green.cr2
        ./alt --setlamps  -s "125 1"   --sticks -n8 -s10
        capture ${ldir}/s1_red.cr2
        ./alt --setlamps  -s "137 1"  --sticks -n8 -s10
        capture ${ldir}/s1_blue.cr2

        # seg 2 : lower
        ./alt --setlamps -s "112 1"   --sticks -n8 -s10  
        capture ${ldir}/s2_green.cr2
        ./alt --setlamps -s "124 1"  --sticks -n8 -s10  
        capture ${ldir}/s2_red.cr2
        ./alt --setlamps -s "136 1"  --sticks -n8 -s10  
        capture ${ldir}/s2_blue.cr2

         

	echo "indep test finished on `date`" | mail -s "ALT finished" n0mfg64@googlemail.com


