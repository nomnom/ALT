# get available camera 
#  gphoto2 --auto-detect  
#then use model string to identify cam


# get abilities
#  gphoto2 --camera "Canon EOS 5D Mark II" --list-config
#

ss=1/200; if [ -n "$1" ] ; then ss=$1; fi
ap=2.8; if [ -n "$2" ] ; then ap=$2; fi

# set default config

gphoto2 --camera "Canon EOS 5D Mark II" \
	--set-config autopoweroff=0 \
	--set-config iso=100 \
 	--set-config whitebalance=1 \
	--set-config aperture=$ap \
	--set-config shutterspeed=$ss

# download image like this: 
gphoto2 --camera "Canon EOS 5D Mark II" \
	--capture-tethered --force-overwrite --filename preview.cr2
	#--capture-image-and-download --force-overwrite --filename preview.cr2

dcraw -4 preview.cr2
rm preview.cr2

pfsv preview.ppm






