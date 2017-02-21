/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * The Source class acquires, linearizes and returns images. Supports threading.
 *
 */

#ifndef SOURCE_HH
#define SOURCE_HH

#include "utils.h"


//! Acquires and linearizes images.
class Source
{

  public:
    
    Source();
    ~Source();
    
    int getWidth () { return width; }
    int getHeight () { return height; }
    imglib::Image<float>& getImage ();
    void setResponseCurve(string);
    void setWhitepoint(rgb wp);
    void setExposure(double exp);
    
    virtual bool hasNewData () = 0;
    virtual void acquire () = 0;

    void start();
    void stop();
    
  protected:  
    
    // threading and locking
    static void* start_thread (void *ptr);
    bool running;
    bool locked;
    
    // latest acquired image
	imglib::Image<float> imageBuffer;       // raw image
	imglib::Image<float> imageCopy;         // w/ applied response curve
    
    // image size
    int width, height;

    
    
    // response curve stuff    
    double exposure;
    rgb whitepoint;
    void loadResponseCurve(string filename);
    void normalizeResponse ();
    double linearize(double value, int channel);
    imglib::Image<float>& linearize(imglib::Image<float>&);
    
    // rgb response curve with variable number of entries
    int responseSize;
    double* responseCurve [3];      
    enum { RESPONSE_LINEAR, RESPONSE_SRGB, RESPONSE_FILE } responseType;
    
    
    // capture rate
    double updateRate;
    
    
};

#endif
