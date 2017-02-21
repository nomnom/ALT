/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Specialization of the source class that grabs images from the X11 desktop.
 *
 */

#include "x11source.h"

using namespace std;


X11Source::X11Source (params c) : config(c) {
    init(); 
} 

X11Source::X11Source (string display, rect area, double rate) {
    config.x11Display = display;
    config.captureArea = area;
    config.updateRate = rate;
    init();
}

X11Source::~X11Source() {
  
    // cleanup
    XShmDetach(dpy, &shminfo);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, NULL);
    XCloseDisplay(dpy);
    XDestroyImage(xImage);
  
}

X11Source::params X11Source::getConfig() { return config; }
    
/**
 * Initialize X11 image grabbing.
 */ 
void X11Source::init ()
{    
    // ask user to select area on his x11 desktop
    if (not config.captureArea.isValid()) { 
        config.captureArea = x11SelectAreaOnDesktop(display);  
    }
    
    
    width = config.captureArea.w;
    height = config.captureArea.h;
    updateRate = config.updateRate;
    
    running=false;
    locked=false;
    

    // open X display
    if(!(dpy = XOpenDisplay(config.x11Display.c_str())) ) { 
        Log().err() << "Error: could not open display";
    } 
    int scr = XDefaultScreen(dpy);
    
    
    // get and attach to shared memory
    xImage = XShmCreateImage(dpy, DefaultVisual(dpy, scr), 
                                  DefaultDepth(dpy, scr), 
                                  ZPixmap, NULL,  
                                  &shminfo, 
                                  config.captureArea.w, config.captureArea.h);
    shminfo.shmid = shmget(IPC_PRIVATE, xImage->bytes_per_line * config.captureArea.h, IPC_CREAT|0777);
    if (shminfo.shmid == -1) {
        // cleanup
        XCloseDisplay(dpy);
        XDestroyImage(xImage);
        Log().err() << "Fatal: Can't get shared memory!";
    }    
    shminfo.shmaddr = (char*)shmat(shminfo.shmid, 0, 0);
    xImage->data = shminfo.shmaddr;
    shminfo.readOnly = False;
    if (!XShmAttach(dpy, &shminfo)) {
        // cleanup
        XCloseDisplay(dpy);
        XDestroyImage(xImage);
        Log().err() << "Fatal: Failed to attach shared memory!";
    }
    
    imageBuffer = imglib::Image<float>(width, height, 3);
    
    
    Log().log(1) << "initialized X11 source:";
    Log().log(1) << "  x11Display = " << config.x11Display; 
    Log().log(1) << "  captureArea = \"" << config.captureArea.x << " " << config.captureArea.y << " " << config.captureArea.w << " " << config.captureArea.h << "\"";
    Log().log(1) << "  updateRate = " << config.updateRate << " s^-1";
}

bool X11Source::hasNewData () { 
    return true; 
}

/**
 * Grabs one image from desktop
 */
void X11Source::acquire ()
{ 
    // grab image from desktop
    if (!XShmGetImage(dpy, RootWindow(dpy, DefaultScreen(dpy)), xImage, config.captureArea.x, config.captureArea.y, AllPlanes)) {
        Log().err() << "Problem accessing shared memory: XShmGetImage() failed";
    }
    
    // convert XImage to our image format
    unsigned long pixel;
    for (int y=0; y<height; y++) {
      for (int x=0; x<width; x++) {
        pixel = XGetPixel(xImage,x,y);          // TODO maybe this can be improved?
        imageBuffer(x,y,0) = (float)(pixel>>16) / 255.0f; 
        imageBuffer(x,y,1) = (float)((pixel&0x00ff00)>>8) / 255.0f;
        imageBuffer(x,y,2) = (float)(pixel&0x0000ff) / 255.0f;
      }
    }
    
}



/**
 * Returns image data straight from X11 shared memory.
 */
char* X11Source::getImageRaw() { 
  
    // grab an image
    if (!XShmGetImage(dpy, RootWindow(dpy, DefaultScreen(dpy)), xImage, config.captureArea.x, config.captureArea.y, AllPlanes)) {
        Log().err() << "Problem accessing shared memory: XShmGetImage() failed";
    }
    return xImage->data;
}


////////// X11  stuff  /////////////////////////////////////////////////

/**
 * Asks the user to select a points on the desktop. Uses X11 functions for displaying a cursor and reacting to the click.
 */
const point X11Source::x11SelectPointOnDesktop (Display *disp)
{
    
   point pos;
  
 // the following window selection function is inspired by Select_Window() in dsimple.h from X11's xwd 1.0.5 program
 // Written by Mark Lillibridge.   Last updated 7/1/87

  int status;
  Cursor cursor;
  XEvent event;
  
  int scr = XDefaultScreen(disp);
  Window target_win = None, root = RootWindow(disp, scr);
  int buttonPressed = 0;
  bool done = false;

  // Make the target cursor
  cursor = XCreateFontCursor(disp, XC_crosshair);

  status = XGrabPointer(disp, root, False,
      ButtonPressMask|ButtonReleaseMask, GrabModeSync,
      GrabModeAsync, root, cursor, CurrentTime);

  if (status != GrabSuccess) Log().err() << "Can't grab the mouse.";
  
  
  
  // react on button clicks
  while (!done) {
    // allow one more event
    XAllowEvents(disp, SyncPointer, CurrentTime);
    XWindowEvent(disp, root, ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
      case ButtonPress: 
        buttonPressed=true;
        // get cursor position
        Window window_returned; 
        int x,y, win_x, win_y;
        unsigned int mask_return;
        XQueryPointer(disp, XRootWindow(disp, 0), &window_returned,
                    &window_returned, &x, &y, &win_x, &win_y,
                    &mask_return);  
        Log().log(1) << "mouse x " << win_x << "   y " << win_y;
        pos.x=x;
        pos.y=y;
      break;
    case ButtonRelease:  if (buttonPressed) { done=true; } break;
    }
  } 

  XUngrabPointer(disp, CurrentTime);  


  return pos;
}


/**
 * Asks the user to select two points on the desktop that define the top left and bottom right corner of the image area to grab.
 */
const rect X11Source::x11SelectAreaOnDesktop (string display)
{
	
    // open X display
    if(!(dpy = XOpenDisplay(display.c_str())) ) { 
      Log().err() << "Can't open X display";
    }
    
    int scr = XDefaultScreen(dpy);
    
    // let user select a topleft and bottom right corner on screen
    Log().msg() << " >> Please select the topleft corner of the capture area.";
    point tl = x11SelectPointOnDesktop(dpy);
    Log().msg() << " >> Please select the bottom right corner of the capture area.";
    point br = x11SelectPointOnDesktop(dpy);
    
    XCloseDisplay(dpy);
    
    return rect (tl.x, tl.y, br.x-tl.x, br.y-tl.y);
    
}
