/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This abstract class represents a group of lamps of identical type (controlled by the same hardware). 
 * A lamp is monochrome and takes a rational value from 0..1 , where 1.0 is the maximum brightness and 0 turns the 
 * lamp off. It provides virtuals for setting lamp values, driving the hardware, automatic fading.
 * It provides threaded updating.
 *
 */

#include "lamps.h"

Lamps::Lamps(){ running = false; };
Lamps::~Lamps(){};


void Lamps::setFadeSpeed (double speed) { fadeSpeed = speed; }
void Lamps::setUpdateRate (double rate) { updateRate = rate; }


/**
 * Does one fading step (if fadespeed > 0) and calls send() at the end.
 */
bool Lamps::doStep (double delta_t)
{
  
    if (fadeSpeed > 0)
        for (int l=0; l<getNumLamps(); l++) {
            double delta = (lampTargetValues[l]-previousLampValues[l]);
            if (abs(delta) > 1e-4) // no fading for very small values
                lampValues[l] += delta / ( (1.0/fadeSpeed)/delta_t );
        }
    
    return send();
}


/**
 *  Stops automatic updating.
 */
void Lamps::stop() { running=false; }

/** 
 * Starts fading thread for automatic updating and fading.
 */
void Lamps::start()
{    
    running = true;
    pthread_t thread;
    pthread_create (&thread, NULL, start_thread, this); 
}

/** 
 * The updating thread.
 */
void* Lamps::start_thread (void *ptr)
{ 
    // get pointer to class
    Lamps *tc = reinterpret_cast<Lamps*>(ptr);
    
    struct timespec update_cycle = { 0, 1e9/tc->updateRate };
    
    while (tc->running) {
        tc->doStep(1.0/tc->updateRate);
        nanosleep(&update_cycle, NULL);
    }
    return 0;
}


int Lamps::getNumLamps() { return numLamps; }

/**
 * Return the current brightness of a single lamp.
 */
 
double Lamps::getValue(int lampID) { return lampValues[lampID]; }

/**
 * Sets the brightness of a lamp. If fading is enabled, it sets the target fade-to value.
 */
void Lamps::setValue(int lampID, double brightness)
{ 
    assert ( (brightness>=0) && (brightness<=1) );
    assert ( (lampID >= 0) && (lampID < numLamps) );
	
    if (fadeSpeed > 0) {
        previousLampValues[lampID] = lampValues[lampID];
        lampTargetValues[lampID] = brightness;
    } else {
        lampValues[lampID] = brightness;
    }
        
}

/**
 * Set all lamps to the specified brightness.
 */
void Lamps::setAll(double brightness)
{
    for (int l=0; l<getNumLamps(); l++) 
        setValue(l,brightness);
}



