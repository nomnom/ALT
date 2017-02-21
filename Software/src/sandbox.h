/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Sandbox for experiments.
 *
 */

#ifndef SANDBOX_HH
#define SANDBOX_HH

#include "utils.h"
#include "gui.h"
#include "source.h"
#include "lightprobe.h"
#include "lamps.h"
#include <unistd.h>

//! A sandbox for experiments.
class Sandbox {
    
  public:
    
    Sandbox(Source* s, Lightprobe* p, Lamps* l) : source(s), probe(p), lamps(l) {};
    ~Sandbox() {};
    
    
    void run();
    
  private:
    
    Source* source;
    Lightprobe* probe;
    Lamps* lamps;

};


#endif
