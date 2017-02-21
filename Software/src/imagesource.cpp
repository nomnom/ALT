/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Specialization of the source class that uses image files. Loads either a single image file or a whole sequence of frames from a directory.
 *
 */

#include "imagesource.h"


/**
 * Constructor checks if config specifies a single image (ending in .ppm) or a directory. It then loads the first image to check for the dimensions.
 */
ImageSource::ImageSource (params c) : config(c), curImageID(0)
{
    // if path ends with .ppm use single image mode
    if (config.imagePath.size() > 4) {
        string suffix = ".ppm";
        if (std::equal(config.imagePath.begin() + config.imagePath.size() - 4, config.imagePath.end(), suffix.begin() ) ) {
            curImageID = -1;
            
            // mode 1 : single image if path ends with ".ppm"
            Log().log(1) << "loading image " << config.imagePath;
            imageBuffer = imglib::Image<float>(config.imagePath);    
        }
    }
    
    // peek at first image so we know the size
    if (curImageID == 0) {
        stringstream ss;
        ss << config.imagePath << "/img_" << curImageID << ".ppm";  
        imageBuffer = imglib::Image<float>(ss.str());
    }

    Source::width = imageBuffer.getWidth();
    Source::height = imageBuffer.getHeight();
    
    
    Log().log(1) << "initialized image source:";
    Log().log(1) << "  image path = " << config.imagePath << endl;
}

ImageSource::params ImageSource::getConfig() { return config; }
    
bool ImageSource::hasNewData () { return true; }

/**
 * Loads either a single image or a whole directory of frames. The latter one requires the files to be named img_#.ppm where # is a number w/0 trailing zeroes.
 */
void ImageSource::acquire () {
    
    
    // mode 2 : load next image named "img_<index>.ppm" from directory
    if (curImageID > -1) {        

        stringstream ss;
        ss << config.imagePath << "/img_" << curImageID << ".ppm";
        
        // check if file exists
        ifstream file (ss.str().c_str());
        if (file) {
            
            imageBuffer = imglib::Image<float>(ss.str());
            curImageID++;
            
        } else {
            
            // loop back to first image
            curImageID = 0;
            acquire();   
        }
    }
}
