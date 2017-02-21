/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Old class for testing and debugging the probe. 
 *
 */

#ifndef TESTPROBE_HH
#define TESTPROBE_HH

#include "lightprobe.h"
#include "source.h"
#include "gui.h"

//! Test light probe (for debug).
class TestProbe { 

  public:
    
    TestProbe (Lightprobe* p) : probe(p) { };
    ~TestProbe();
    
    void run();
    
    
  private:

    Lightprobe* probe;

};


#endif
