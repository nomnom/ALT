/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * For maximizing the exposure of an UVC controlled webcam.
 *
 */

#include "maxexposure.h"


const string MaxExposure::videoDevice = "/dev/video0";

MaxExposure::MaxExposure(Source* s) : source(s) {
    
    // activae manual exposure settings
    stringstream cmd;
    cmd << "uvcdynctrl -d " << videoDevice << " -s \"Exposure (Auto)\" 1";
    system(cmd.str().c_str());
}

void MaxExposure::run()
{

    Log().msg() << "running probe exposure maximization routine";
    double maxAllowed = 0.01; // 1	 percent
    
    double rate=1;
    
    imglib::Image<float> img;
    
    Gui gui(source->getWidth(), source->getHeight());
    
    Log().msg() << "please mark the area with a circle";
    circle area = gui.runSphereSelection(source);
    
    
    
    //
    // find highest possible exposure
    //
    const double minExposure = 0;
    const double maxExposure = 100;
    double exposure =  maxExposure;
    while (true) {
        Log().msg() << "trying exposure " << (int)exposure;
        
        // set camera exposure
        
        setExposure ((int)exposure);
        
        // wait for cam
        usleep((long)( (1/rate)*1000000));
    
        // capture raw image
        source->acquire();
        img = source->getImage();
        
        // count overexposed pixels in region
        int numOverexposed=0;
        int numPixels=0;
		
		// sum up values in specified area and average
		for (int y=area.y-area.r; y<area.y+area.r; ++y)
			for (int x=area.x-area.r; x<area.x+area.r; ++x)
				if ((area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) < area.r*area.r) {
					numPixels ++;
					for (int c=0; c<3; c++) {
						if (img (x,y,c) >= 1.0) {
							numOverexposed ++;
							break;
						}
					}
				}
				
					
        
        Log().msg() << "-> results in " << (double)numOverexposed << " overexposed pixels";
        
        // TODO maybe use bisection algo
        // if percentage is too high, reduce exposure by 5% and repeat
        if (  ((double)numOverexposed / (double)numPixels) > maxAllowed) {
            exposure *= 0.95;
            Log().msg() << "-> reducing exposure";
        } else {
            Log().msg() << "-> accepting exposure of " << (int)exposure;
            break;;
        }
    }
        
        
        
   this->exposure = exposure;

}


/**
 * Set exposure on uvc video device.
 */
void MaxExposure::setExposure (int exp)
{
    stringstream cmd;
    cmd << "uvcdynctrl -d " << videoDevice << " -s \"Exposure (Absolute)\" " << int(exp);
    Log().log(1) << cmd.str();	
    system(cmd.str().c_str());
}
