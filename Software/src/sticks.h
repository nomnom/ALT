/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This class represents our  lighting system. Controls eight strips of 120 WS2812 LEDs via the Teensy microcontroller over the serial port.
 * LEDs on a strip are segmented into a equal sized patches.
 *
 */

#ifndef STICKS_HH
#define STICKS_HH

#include "lamps.h"

#include "utils.h"

#include <string>
#include <iostream>
#include <stdlib.h>     // C standard library
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitionss
#include <vector>       // vectors
#include <pthread.h>
#include <time.h>

using namespace std;

//! Our sticks lighting system.
class Sticks : public Lamps
{
    
  public:
  
  
    //! Configuration of our lighting system.
    struct params {
        params() : segmentSize(30), numSticks(8), stickSize(120), fadeSpeed(1.0/30.0), serialDevice("/dev/ttyACM0"), 
                   updateRate(30) {}
        int segmentSize;
        int numSticks;
        int stickSize;
        double fadeSpeed;
        string serialDevice; 
        double updateRate;
        void load (string file);
    };
  
    Sticks (params config);
    Sticks (int segmentSize, string device, double rate);
    ~Sticks();
    
    
    params getConfig();
    
    // address RGB lamps
    bool hasRGBLamps();
    int getNumRGBLamps();
    void setRGBValue(int lampID, rgb);
    rgb getRGBValue(int lampID);
    void setAllRGB(rgb);
    
    // address as RGB sticks (one stick per channel);
    int getNumSticks();
    int getStickLength(int lampID);
    void setStickRGBValue(int stickID, int lampID, rgb);
    rgb getStickRGBValue(int stickID, int lampID);
    void setStickChannelValue(int stickID, int stickLampID, double val, int channel);
    void setAllChannel(double val, int channel);
    
    
    
    // realworld interface
    bool isReady() { return (serialPort != -1); }
    bool send();
    
  
  private:
  
    void init();
  
    params config;
    
    
    // led strip layout
    int stripLengths[8];
    int maxStripLength;
    int realStripLength;
    int maxNumSegs;
    
    // raw data of all segments [stickID][colorChannel]
    vector<unsigned char> rawValues[8][3];
    unsigned char mapMono ( double brightness );
    
    int serialPort; // filehandle
};


#endif
