/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Old class for testing and debugging the lamps. 
 *
 */


#ifndef TESTLAMPS_HH
#define TESTLAMPS_HH

#include "lamps.h"
#include "source.h"
#include "gui.h"
#include "lamppool.h"
#include "sticks.h"

//! Test lamps (for debug).
class TestLamps { 

  public:
    
    TestLamps (Lamps* l, Source* s) : lamps(l), source(s) { };
    ~TestLamps();
    
    void run();
    
    
  private:

    Lamps* lamps;
    Source* source;
    

};


#endif
