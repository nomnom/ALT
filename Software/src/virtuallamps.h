/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This class represents a group of virtual monochrome lamps
 *
 */

#ifndef VIRTUALLAMPS_HH
#define VIRTUALLAMPS_HH


#include "lamps.h"

#include "utils.h"

#include <string>
#include <iostream>
#include <vector>       // vectors
#include <pthread.h>
#include <time.h>

using namespace std;

//! Lamps w/o hardware backend.
class VirtualLamps : public Lamps
{
    
  public:
    
    VirtualLamps (int numLamps);
    ~VirtualLamps() {};
        
    // realworld interface
    bool isReady();
    bool send();
  
};


#endif
