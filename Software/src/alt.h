/**
 * @author Manuel Jerger <nom@nomnom.de>
 *
 */

// utils
#include "utils.h"
#include "gui.h"

// components
#include "lamps.h"
#include "lamppool.h"
#include "virtuallamps.h"
#include "sticks.h"
#include "x11source.h"
#include "lightprobe.h"

// independent program modes
#include "transfer.h"
#include "calibrate.h"
#include "testlamps.h"
#include "testprobe.h"
#include "maxexposure.h"
#include "sandbox.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <getopt.h> 

#include <X11/Xlib.h>

#include <execinfo.h>
#include <signal.h>

using namespace Eigen;

// master arguments for parsing commandline args
// also partially used for program mode and source type
enum {  TRANSFER, 
        TRANSFER_SAMPLER,
        CAPTURE, 
        CALIBRATE_LAMPS,
        TESTLAMPS,
        TESTPROBE, 
        SETLAMPS, 
        MAX_EXPOSURE,
        SANDBOX,
        SAMPLE_UNI_OLD,
        SAMPLE_UNI,
        SAMPLE_FILE,
        SAMPLE_ALL,
        X11SOURCE, 
        IMAGESOURCE, 
        STICKS, 
        VIRTUAL_LAMPS,
        LIGHTPROBE };
       
