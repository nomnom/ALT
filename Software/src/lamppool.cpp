/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * This class represents a pool of lamps. It controls multiple Lamps classes and behaves like a single lamp class.
 *
 */
#include "lamppool.h"


LampPool::LampPool () { 
    Lamps::numLamps = 0;
}

int LampPool::getNumMembers() { return members.size(); }

Lamps* LampPool::getMember( int m ) {
    return members[m];
}
void LampPool::addMember ( Lamps* lamps ) {
    members.push_back(lamps);
    Lamps::numLamps += lamps->getNumLamps();
}

bool LampPool::isReady() { 
    bool ready = false;
    for (int i=0; i<members.size(); i++) {
        ready |= members[i]->isReady();
    }
    return ready;
}
    
bool LampPool::send() {
    
    bool success = true;
    for (int i=0; i<members.size(); i++) {
        success &= members[i]->send();
    }
    return success;
}

int LampPool::getMemberForLampIndex(int lampID)
{
    for (int i=0; i<members.size(); i++) {
        if (lampID < members[i]->getNumLamps()) { 
            return i;
        } else {
            lampID -= members[i]->getNumLamps();
        }
    }
    Log().err() << " lamp index out of range";
    return -1;
}

int LampPool::getMappedLampIndex(int lampID)
{
    for (int i=0; i<members.size(); i++) {
        if (lampID < members[i]->getNumLamps()) { 
            return lampID;
        } else {
            lampID -= members[i]->getNumLamps();
        }
    }
    Log().err() << " lamp index out of range";
    return -1;
}


int LampPool::getNumLamps() { return Lamps::numLamps; }
void LampPool::setValue(int lampID, double value) {
    members[getMemberForLampIndex(lampID)]->setValue(getMappedLampIndex(lampID), value);
}
double LampPool::getValue(int lampID) {
    return members[getMemberForLampIndex(lampID)]->getValue(getMappedLampIndex(lampID));
}
void LampPool::setAll(double value) {
    for (int i=0; i<members.size(); i++) {
        members[i]->setAll(value);
    }
}
bool LampPool::doStep (double delta_t) {
    for (int i=0; i<members.size(); i++) {
        members[i]->doStep(delta_t);
    }
}
void LampPool::setFadeSpeed (double speed) {
    Lamps::fadeSpeed = speed;
    for (int i=0; i<members.size(); i++) {
        members[i]->setFadeSpeed(speed);
    }
}
void LampPool::setUpdateRate (double rate) {
    Lamps::updateRate = rate;
    for (int i=0; i<members.size(); i++) {
        members[i]->setUpdateRate(rate);
    }
}
