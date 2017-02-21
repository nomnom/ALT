/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This class represents a group of virtual monochrome lamps
 *
 */

#include "virtuallamps.h"

VirtualLamps::VirtualLamps (int numLamps) { 
    Lamps::numLamps = numLamps;
    
    lampValues.resize(Lamps::numLamps);
    previousLampValues.resize(Lamps::numLamps);
    lampTargetValues.resize(Lamps::numLamps);
}
    


bool VirtualLamps::isReady() { return true; }
    
bool VirtualLamps::send() { return true; }
    
    
