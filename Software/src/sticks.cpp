/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This class represents our  lighting system. Controls eight strips of 120 WS2812 LEDs via the Teensy microcontroller over the serial port.
 * LEDs on a strip are segmented into a equal sized patches.
 *
 */



#include "sticks.h"


Sticks::Sticks (params c) : config(c)
{
    init();
}

Sticks::Sticks (int segmentSize, string serialDevice, double rate)
{
    config.segmentSize = segmentSize;
    config.serialDevice = serialDevice;
    config.updateRate = rate;
    
    init();
}


void Sticks::params::load(string file)
{
            
    ifstream in;
    in.open(file.c_str(), ios::in);
    string tmp;
    in >> tmp;
    if (tmp != "config_sticks1") { 
        Log().err() << "config file " << file << " is not compatible with this sticks class"; 
    } else {
        in >> stickSize;
        in >> numSticks;
        in >> segmentSize;
        Log().log(0) << "successfully loaded sticks config " << file  << endl;
    }
    in.close();

}

void Sticks::init ()
{ 
	
    Log().log(1) << "initializing sticks with:";
    Log().log(1) << "  fadeSpeed = " << config.fadeSpeed;
    Log().log(1) << "  serialPort = " <<  config.serialDevice; 
    Log().log(1) << "  segmentSize = " << config.segmentSize << " LEDs / segment";
    Log().log(1) << "  numSticks = " << config.numSticks << " of 8 sticks";   
    //
    // setup the basic lamp configuration
    //
    
    // physical length (so if we use less leds, the rest get 0's instead of no data)
    realStripLength=120;
    
    // length of strip used // all are of the same length
    maxStripLength=config.stickSize;
    for (int i=0; i<8; i++) { 
        stripLengths[i] = maxStripLength; 
    }
    
    // total num lamps => all 8 sticks
    int numLampsTotal = 0; 
    for (int s=0; s<8; s++) { 
        numLampsTotal += 3 * stripLengths[s] / config.segmentSize;
    }
    lampValues.resize(numLampsTotal);
    previousLampValues.resize(numLampsTotal);
    lampTargetValues.resize(numLampsTotal);
    
    
    // virtual numLamps -> only the sticks that are in use
    Lamps::numLamps = 0;
    for (int s=0; s<config.numSticks; s++) { 
        Lamps::numLamps += 3 * stripLengths[s] / config.segmentSize;
    }
    
    
    int lampIdx = 0;
    maxNumSegs = 0;
    
    setFadeSpeed(config.fadeSpeed);
    setUpdateRate(config.updateRate);
    
    
    // for each stick .. 
    for (int s=0; s<8; s++) { 
    
        // .. and for each color ..
        for (int c=0; c<3; c++) { 
            
            // ... init raw array and lampValues
            rawValues[s][c] = vector<unsigned char>();
            
            int subSegmentIdx=0;
            for (int l=0; l<stripLengths[s]; l++) {    
                
                // create one lamp per segment
                subSegmentIdx++;
                if (subSegmentIdx == config.segmentSize) {
                    subSegmentIdx=0;
                    lampValues[lampIdx]=0;
                    previousLampValues[lampIdx]=0;
                    rawValues[s][c].push_back(0);
                    lampIdx++;
                } 

            }
       
            if (rawValues[s][c].size() > maxNumSegs) 
                maxNumSegs = rawValues[s][c].size();
        }        
    }
    
    //
    // open and configure serial port
    //
    
    serialPort = open(config.serialDevice.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serialPort < 0 ) {
      Log().err() << "could not open serial port";
      return;
    }
      

    // set serialport to 115200 baud, 8N1
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (serialPort, &tty) != 0) {
        Log().err() << "could not get serial config from system";
        close (serialPort);
        return;
    }
    

    cfsetospeed (&tty, B115200);
    cfsetispeed (&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // ignore break signal
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);        // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (serialPort, TCSANOW, &tty) != 0)
      Log().err() << "could not configure serial port";
 
    
    //TODO:  test if below code is sufficient
    
    /*struct termios port_settings;
    cfsetispeed(&port_settings, B115200);    
    cfsetospeed(&port_settings, B115200);
    port_settings.c_cflag &= ~PARENB;
    port_settings.c_cflag &= ~CSTOPB;
    port_settings.c_cflag &= ~CSIZE;
    port_settings.c_cflag |= CS8;
    if (tcsetattr(serialPort, TCSANOW, &port_settings)  == -1 ) {
      err ("error configuring serial port",false);
    }  */
 
    
 
}


Sticks::~Sticks()
{
    // close serial port (if any)
    if (serialPort != -1) {
        close(serialPort);
        serialPort = -1;
    }
}


Sticks::params Sticks::getConfig() { return config; }

bool Sticks::hasRGBLamps() { return true; }


// map float brightness to LED raw 8-bit values
unsigned char Sticks::mapMono ( double brightness )
{
    return (unsigned char) (brightness * 255.0 + 0.5);
    
}

// address RGB lamps
int Sticks::getNumRGBLamps() { return getNumLamps() / 3; }
void Sticks::setRGBValue(int lampID, rgb color)
{
    assert ( (lampID<getNumLamps() / 3) && (lampID>=0));
    int segsPerStick = maxStripLength / config.segmentSize;
    // calculate stick and lamp on stick, then use getStickRGBValue() to get the value
    int stickLampID = lampID % (segsPerStick);              // lamp index on its stick
    int stickID = (lampID / segsPerStick);    // stick index // TODO fix
    cout << stickID << " " << stickLampID << endl;
    setStickRGBValue(stickID, stickLampID, color);        
}
    
rgb Sticks::getRGBValue(int lampID)
{   
    assert ( (lampID<getNumLamps() / 3) && (lampID>=0));
    int segsPerStick = maxStripLength / config.segmentSize;
    // calculate stick and lamp on stick, then use getStickRGBValue() to get the value
    int stickLampID = lampID % (segsPerStick);              // lamp index on its stick
    int stickID = (lampID / segsPerStick);    // stick index // TODO fix
    return getStickRGBValue(stickID, stickLampID);
}
    
// address RGB lamps per stick
int Sticks::getNumSticks() { return config.numSticks; }
int Sticks::getStickLength(int stickID) { return stripLengths[stickID] / config.segmentSize; }

// set rgb value of a segment (=lamp) on a specific channel (=stick)
void Sticks::setStickRGBValue(int stickID, int stickLampID, rgb color)
{
    assert ( (stickID<getNumSticks()) && (stickID>=0));
    assert ( (stickLampID<getStickLength(stickID)) && (stickLampID>=0));
    
    int segsPerStick = maxStripLength / config.segmentSize;
    
    // RGB configuration
    //for (int c=0; c<3; c++) 
	//	setValue(3*segsPerStick*stickID + c*segsPerStick + lampID, color[c]);	
    
    
    // GRB configuration (most WS2812 LEDs are wired this way)
    setValue(3*segsPerStick*stickID + 0*segsPerStick + stickLampID, color.g);	
    setValue(3*segsPerStick*stickID + 1*segsPerStick + stickLampID, color.r);
    setValue(3*segsPerStick*stickID + 2*segsPerStick + stickLampID, color.b);
}


rgb Sticks::getStickRGBValue(int stickID, int stickLampID)
{
    assert ( (stickID<getNumSticks()) && (stickID>=0));
    assert ( (stickLampID<getStickLength(stickID)) && (stickLampID>=0));
    
    rgb color;
    int segsPerStick = maxStripLength / config.segmentSize;
    
    
    // RGB configuration
    //for (int c=0; c<3; c++) 
	//	setValue(3*segsPerStick*stickID + c*segsPerStick + lampID, color[c]);

    // GRB configuration (most WS2812 LEDs are wired this way)    
    color.g = getValue(3*segsPerStick*stickID + 0*segsPerStick + stickLampID);
    color.r = getValue(3*segsPerStick*stickID + 1*segsPerStick + stickLampID);
    color.b = getValue(3*segsPerStick*stickID + 2*segsPerStick + stickLampID);
    return color;
}
    
// set rgb color of all lamps at once
void Sticks::setAllRGB(rgb color)
{
    for (int s=0; s<getNumSticks(); s++)
        for (int l=0; l<getStickLength(s); l++)
            setStickRGBValue(s,l,color);
}


// set single channel value of a segment (=lamp) on a specific stick
void Sticks::setStickChannelValue(int stickID, int stickLampID, double val, int channel)
{
    assert ( (stickID<getNumSticks()) && (stickID>=0));
    assert ( (stickLampID<getStickLength(stickID)) && (stickLampID>=0));
    
    int segsPerStick = maxStripLength / config.segmentSize;
    
    // GRB configuration (most WS2812 LEDs are wired this way)
    setValue(3*segsPerStick*stickID + channel*segsPerStick + stickLampID, val);	

}


void Sticks::setAllChannel(double val, int channel) {
    
    for (int s=0; s<getNumSticks(); s++)
        for (int l=0; l<getStickLength(s); l++)
            setStickChannelValue(s,l,val, channel);
    
}

// write out serial data and set the realworld lamps
bool Sticks::send()
{
    
    // update (segmented) raw values from lamp values
    int lampIdx=0;
    for (int s=0; s<8; s++) { 
        stringstream ss;
        for (int c=0; c<3; c++) { 
            for (int v=0; v<rawValues[s][c].size(); v++) {
                rawValues[s][c][v] = mapMono(lampValues[lampIdx]);
                ss <<  (int)rawValues[s][c][v] << " ";
                lampIdx++;  
            }
        }
        Log().log(3) << ss.str();
    }
    
    // convert raw data to output format (which is stickwise interlaced, unsegmented)
    int dataLen = 1 + realStripLength*8*3;
    unsigned char data[dataLen];
    data[0]='*'; // first char is a mode indicator
    int segIdx=0;
    for (int segIdx=0; segIdx<maxNumSegs; segIdx++) {
        
        // 
        
        // create 24 bytes from each segment
        int data24[24];
        int byte=0;
        for (int c=0; c<3; c++) {         // 3 color channels
            for (int b=0; b<8; b++) {     // 8 bit per channel
                byte=0;
                for (int s=0; s<8; s++) { // 8 led channels
                
                    // b't byte contains the b'th bit of all led channels
                    if ( (1 << (7-b) ) & rawValues[s][c][segIdx]) {   
                        byte |= (1 << s);
                    } 
                }
                data24[c*8 + b] = byte;
            }
        }
       
        // copy data for all LEDs in segment 
        for (int subSegIdx=0; subSegIdx<config.segmentSize; subSegIdx++)
            for (int b=0; b<24; b++)
                data[1 + segIdx*config.segmentSize*24 + subSegIdx*24 + b] = data24[b];
    }
    
    // fill remaining data with 0s (if realstriplen < maxstriplen)
    for (int i=1+maxNumSegs*config.segmentSize*24; i < dataLen; ++i) {
        data[i]=0;
    }
    
    
    
    // write to serial device
    if (serialPort != -1) {
        int bytes_written =  write (serialPort,data,dataLen);   // TODO name clashes with Sticks::write() -> use newer C++ serial library
         if (bytes_written < dataLen ) {
             Log().err() << "data transfer error: only " << bytes_written << " of " << dataLen << " bytes arrived";
             return false;
         }
        
    } else {
        Log().log(2) << "did not write : no serial device";
    }
    return false;
}


