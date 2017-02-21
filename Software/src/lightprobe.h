/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Our light probe model based on reflection mapping using a single sphere and a pinhole camera model.
 * Also supports images created with the parametrization from Debevec  http://www.pauldebevec.com/Probes/
 *
 */

#ifndef LIGHTPROBE_HH
#define LIGHTPROBE_HH

#include "utils.h"

#include "source.h"
#include "x11source.h"
#include "imagesource.h"

#include <iostream>
#include <vector>
#include <Eigen/Core>
#include <Eigen/Geometry>

using namespace Eigen;

//! Our light probe model.
class Lightprobe
{

  public:
  
    //! Configuration of the light probe.
    struct params {
        params () : camDistance(17), sphereRadius(4), sphereCircle(circle()), 
                    gamma(0), exposure(0), whitepoint (rgb(1,1,1)),
                    rotation(Vector3d(0,0,0)), horizonAngle(00), responseCurve(""), type(0) {}
        double camDistance;         // distance from camera lense to center of sphere
        
        double sphereRadius;        // size of the lightprobe's sphere
        circle sphereCircle;        // the circle that selects the sphere on the input image
        double gamma;               // deprecated
        rgb whitepoint;
        double exposure;
        Vector3d rotation;          // rotation of the setup around the sphere center
        double horizonAngle;        // lowest sampling angle below horizon
        string responseCurve;       // path to rgb response curves (.m file from pfshdrcalibrate) or LINEAR or simple list w/ 3 columns for r/g/b7
        string maskFile;            // path to mask image. only pixels with value > 0 are taken into account while sampling
        int type;                   // 0 = pinhole sphere mapping;  1 = debevec mapping
        void load (string file);
        void save (string file);
    };

    //! Configures sampling.
    struct samplingParams {
        samplingParams () : numSamplesH(5), numSamplesA(8), numSamples(100), coneSize(M_PI*2/8),
                            coneSigma(0.5), filename(""), samplingMode(UNIFORM), kernelMode(NEIGHBOR), minConeSize(0) {} 
        
        enum { UNIFORM_OLD, UNIFORM, FROM_FILE, ALLPIXELS }  samplingMode;
        
        // for UNIFORM_OLD
        int numSamplesH;
        int numSamplesA;
        
        // uniform sampling with fixed number of samples
        int numSamples;
        
        // directions from file 
        string filename;

        enum { NEIGHBOR, GAUSS, NONE } kernelMode;        
        double coneSize;        // gauss: how large the cone is (rest gets clipped)
        double coneSigma;       // gauss: sigma value (may be calculated automatically)
        
        int minConeSize;     // directions with less pixels get deleted
    };


    Lightprobe (Source*, params c, samplingParams sampling);
    Lightprobe (Source*, string configFile,  samplingParams sampling);
    Lightprobe (Source*, params c, samplingParams sampling, directions samplingDirs);
    ~Lightprobe ();
    
    // getter / setter
    params getConfig();
    samplingParams getSamplingConfig();
    Source* getSource();
    
    // data aquisition
    void acquire ();
    imglib::Image<float>& getImage ();
    bool hasNewData ();
    
    // rotate probe (forces recalculation)
    void setRotationY ( double rad );
    
    // some precalc functions
    void precalculateDirectionPixelData ();
    void precalculateSamplingDirections();
    void precalculateSamplingStructure ();
    void precalculateSamplingCones ();
    void precalculateSamplingNearestNeighbors ();
	void precalculateSamplingAllPixels ();
    
    // sample all precalculated directions and calculate impact 
    vector<rgb> getImpact ();
    vector<rgb> getImpact (imglib::Image<float>& img);
    
    // lightprobe sphere map model: map pixels to light directions
    Vector3d getDirectionFromPixel ( Vector2i pos );
    
    Vector3d getDirectionFromPixelDebevec ( Vector2i pos );
    

  private:
  
    params config;
    samplingParams samplingConfig;
    
    void init();
  
    Source* source;             // source for input images
    imglib::Image<float> maskImage;  // mask image
    
    double planeShift;      // height shift of the image plane w.r.t sphere center in real world 
                            // (the imageplane should be placed exactly where the marked circle in the image touches the sphere in real world)
    
    
    
  // precomputed data
  public:
    
    // direction<>pixel data
    vector<Vector2i> allPixels;         // all available pixels on input image of the sphere..
    directions allDirs;                 // .. and the corresponding directions in global coordinates

    directions samplingDirs;            // light directions that will be sampled ..
    vector<dirCone> samplingCones;      // .. and their precalculated neighborhood in pixel space
    vector<bool> usedDirections;        // true if sampling cone is empty
    
};

#endif
