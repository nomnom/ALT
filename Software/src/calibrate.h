/**
 * @author Manuel Jerger <nom@nomnom.de>
 * The Calibration Loop for use with the webcam probe. Can also measure the response curve of our lighting system.
 *
 */
 
#ifndef CALIBRATE_HH
#define CALIBRATE_HH

#include "utils.h"
#include "lamps.h"
#include "lightprobe.h"
#include "gui.h"

#include "ceres/ceres.h"
#include <glog/logging.h>

//! The Calibration Loop.
class Calibrate { 

  public:
    
    //! Configuration of Calibrate class.
    struct params {
        params() : dataDir("img"), captureRate(1) {}
        string dataDir;
        double captureRate;
    };
    
    Calibrate (Lightprobe* p, Lamps* l, params c);
    Calibrate (Lightprobe* p, Lamps* l, string path, double rate);
    ~Calibrate();
    
    params getConfig();
    
    void runCaptureImpacts();
    void runCalibrateLamps();
    
  private:
    
        
    params config;

    Lightprobe* probe;
    Lamps* lamps;
};


#endif
