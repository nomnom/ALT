/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Specialization of the source class that grabs images from the X11 desktop.
 *
 */

#ifndef X11SOURCE_HH
#define X11SOURCE_HH


#include "utils.h"

#include "source.h"
#include "image.h"

// for screengrab and window selection
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

//! X11 desktop grabber.
class X11Source : public Source
{

  public:
    
    //! Configuration of X11Source.
    struct params {
        params() : x11Display(":0"), captureArea(rect()), updateRate(10) {}
        string x11Display;
        rect captureArea;
        double updateRate;
    };
  
    X11Source (params c);    
	X11Source (string display, rect area, double rate);
    ~X11Source();
    
    params getConfig();
    
    void acquire();
    bool hasNewData();
    
    char* getImageRaw ();
    
    const rect x11SelectAreaOnDesktop (string display);
    const point x11SelectPointOnDesktop (Display *disp);
    
  private:
  
    void init (); 
  
    params config;
  
	string display;
    Display *dpy;
    XShmSegmentInfo shminfo;
    XImage *xImage;
	
    
  
};

#endif
