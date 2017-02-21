/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This abstract class represents a group of lamps of identical type (controlled by the same hardware). 
 * A lamp is monochrome and takes a rational value from 0..1 , where 1.0 is the maximum brightness and 0 turns the 
 * lamp off. It provides virtuals for setting lamp values, driving the hardware, automatic fading.
 * It provides threaded updating.
 *
 */

#ifndef LAMPS_HH
#define LAMPS_HH


#include "utils.h"
#include <vector>

using namespace std;

//!A monochrome lamp.
class Lamps
{
    
  public:
  
    Lamps();
    ~Lamps();
    
    virtual int getNumLamps();
    virtual void setValue(int, double);
    virtual double getValue(int);
    virtual void setAll(double);
    
    void start();
    void stop();
    virtual bool doStep (double delta_t);
    virtual void setFadeSpeed (double speed);
    virtual void setUpdateRate (double rate);
    
    virtual bool isReady() = 0;
    virtual bool send() = 0;
    
  protected:
    
    static void* start_thread (void *ptr);
    bool running;
    
    int numLamps;
    vector<double> lampValues;  		// current lamp values; in the order: foreach channel : foreach color : foreach segment
    vector<double> previousLampValues;  // previous lamp values;
    vector<double> lampTargetValues;  	// fade-to values ; order as above
    
    
    double fadeSpeed;
    double updateRate;
    
};


#endif
