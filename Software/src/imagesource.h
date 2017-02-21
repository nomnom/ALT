/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Specialization of the source class that uses image files. Loads either a single image file or a whole sequence of frames from a directory.
 *
 */
 
#ifndef IMAGESOURCE_HH
#define IMAGESOURCE_HH

#include "source.h"
#include "image.h"
#include <string>

//! Source that uses image files.
class ImageSource : public Source
{

  public:
  
    //! Configuration of the ImageSource class.
    struct params {
        params() : imagePath("") {}
        string imagePath;
    };
  
    ImageSource (params c);   
    ~ImageSource() {};
    
    params getConfig();
    
    bool hasNewData ();
    void acquire ();
    
    
  private:
    
    params config;
    
    int curImageID;
    
};

#endif
