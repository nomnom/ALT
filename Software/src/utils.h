/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Utility functions and important datastructures.
 *
 */


#ifndef UTILS_HH
#define UTILS_HH

#include "image.h"

#include <string>
#include <sstream>
#include <iostream>
//#include <ctime>
#include <cmath>

#include <vector>
#include <Eigen/Core>

// for screengrab and window selection
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

using namespace std;
using namespace Eigen;

//////////  logs and errors  ///////////////////////////////////////////

void err (string msg, bool critical);

//! A simple logger.
class Log {
  public:
    Log(){};
    virtual ~Log();
    ostringstream& log (int lvl);      // for debug logging
    ostringstream& msg ();             // same as log(0)
    ostringstream& err ();             // same as log(0) but message is also printed to stderr
  private:
    ostringstream ss;
    int level;
};


//////////  own data structures  /////////////////////////////////////////////////
    
    
    
//
// environment mapping; efficient datastructures for handling incoming light and directions
//

typedef vector<Vector3d> directions;

//! Stores the neighborhood of one sampling direction.
struct dirCone    
{
    Vector3d direction;         // direction (center of the cone)
    directions dirs;            // light directions of neighbors
    vector<Vector2i> pixels;    // matching pixel positions of neighbors
    
    vector<double> weights;     // weights of the directions
    double weightSum;           // for correct integration
    dirCone(){};
    dirCone (Vector3d dir) : weightSum(0), direction(dir) {}
    void add (Vector3d dir, Vector2i pixel, double weight) {
        dirs.push_back (dir);
        pixels.push_back (pixel);
        weights.push_back (weight);
        weightSum += weight;
    }
};

//! RGB color
struct rgb { 
	double r; double g; double b; 
	rgb () : r(0), g(0), b(0) {};
	rgb (double r, double g, double b) : r(r) , g(g), b(b) {}
	rgb (vector<double> vec) : r(vec[0]) , g(vec[1]) , b(vec[2]) {}
	vector<double> getVec () { 
		vector<double> vec (3);
		vec[0]=r;  vec[1]=g;  vec[2]=b; 
		return vec;
	}
};
	
//! A circle.    
struct circle {
  double x;
  double y;
  double r;
  circle (double x, double y, double r) : x(x), y(y), r(r) {}
  circle () : x(0), y(0), r(0) {}
  bool isValid () { return (r>0); }
};

//! A rectangle.
struct rect {
  double x;
  double y;
  double w;
  double h;
  rect (double x, double y, double w, double h) : x(x), y(y), w(w), h(h) {}
  rect () : x(0), y(0), w(0), h(0) {}
  bool isValid () { return (w>0) && (h>0); }
};

//! A point.
struct point {
  double x;
  double y;
  point (double x, double y) : x(x), y(y) {}
  point() : x(0), y(0) {}
};






////////// IMAGE helpers  //////////////////////////////////////////////



imglib::Image<float>& imgCircularCrop(imglib::Image<float>& imgIn, circle area);


// add/subtract images
imglib::Image<float>& imgAdd(imglib::Image<float>& A, imglib::Image<float>& B);
imglib::Image<float>& imgSub(imglib::Image<float>& A, imglib::Image<float>& B);
imglib::Image<float>& imgAdd(imglib::Image<float>& A, rgb color);


// multiply by scalar
imglib::Image<float>& imgMul(imglib::Image<float>& A, float scalar);


//get max/min pixel value
float imgMax(imglib::Image<float>& A);
float imgMin(imglib::Image<float>& A);

// scale image so max value is 1 (over all channel at once)
imglib::Image<float>& imgScale(imglib::Image<float>& A);

                     
// simple gauss kernel sampling; cannot handle boundary conditions
rgb sampleGauss7 (imglib::Image<float> image, int xpos, int ypos);

//////////  MISC   ////////////////////////////////////////////

// normal distribution
double normalDistribution (double sigma, double mu, double x);


//////////  COLOR and TONEMAPPING  /////////////////////////////////////

rgb mapGamma (rgb value, double gain, double lambda);
double mapLinear (double val, double wp, double bp);
rgb mapLinear (rgb value, rgb whitepoint, rgb blackpoint);


// linear RGB to sRGB
rgb rgb2srgb ( rgb linear );
double rgb2srgb_component ( double value );

// sRGB to linear RGB
rgb srgb2rgb ( rgb sRGB );
double srgb2rgb_component ( double value );

// RGB to xyY
Vector3d rgb2xyY (rgb val);

double clamp (double val);
rgb clamp (rgb val);

//////////  GEOMETRIC   ////////////////////////////////////////////

circle getCircumCircle (Vector2d, Vector2d, Vector2d);
    
directions sampleHemisphere (int, int);
directions sampleSphere (int, int, double );
directions sampleUniform (int numSamples, double horizonAngle);
directions samplesFromFile ( string path, int numVecs, double horizonAngle);
directions loadVectors3d ( string path, int numVecs);
int findNearestNeighbor(Vector3d vec, directions candidates);
Vector2d cartesian2spherical (Vector3d cartesian);
Vector3d spherical2cartesian (Vector2d spherical);
double angle (Vector2d p1, Vector2d p2);


#endif
