/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * For maximizing the exposure of an UVC controlled webcam.
 *
 */

#ifndef MAX_EXPOSURE_HH
#define MAX_EXPOSURE_HH

#include "utils.h"
#include "gui.h"
#include "source.h"
#include <unistd.h>


//! Adjusts the Exposure of a UVC webcam.
class MaxExposure {
    
  public:
  
    MaxExposure (Source* s);
    ~MaxExposure () {};
    
    
    void run();
    
    int getExposure() { return exposure; }
     
    void setExposure(int);
    
  private:
    
    Source* source;
    
    int exposure;

    static const string videoDevice;

};


#endif
