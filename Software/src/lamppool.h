/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This class represents a pool of lamps. It controls multiple Lamps classes and behaves like a single lamp class.
 *
 */

#ifndef LAMPPOOL_HH
#define LAMPPOOL_HH


#include "lamps.h"

#include "utils.h"

#include <string>
#include <iostream>
#include <vector>       // vectors
#include <pthread.h>
#include <time.h>

using namespace std;


//! Groups instances of Lamps.
class LampPool : public Lamps
{
    
  public:
    
    LampPool ();
    ~LampPool() {};
    
    // overload lamp addressing methods
    int getNumLamps();
    void setValue(int, double);
    double getValue(int);
    void setAll(double);
    bool doStep (double delta_t);
    void setFadeSpeed (double speed);
    void setUpdateRate (double rate);
    
    
    void start();
    void stop();
        
    // realworld interface
    bool isReady();
    bool send();
    
    int getNumMembers();
    Lamps* getMember( int m );
    void addMember ( Lamps* lamps );
  
  private:
  
    vector<Lamps*> members;
    int getMemberForLampIndex(int);
    int getMappedLampIndex(int);
};


#endif
