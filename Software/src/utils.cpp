/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Utility functions and important datastructures.
 *
 */

#include "utils.h"
#include <getopt.h>      

//////////  logs and errors  ///////////////////////////////////////////

// enables extended debug logging
int verbosity = 0;

// destructor handles the actual logging
Log::~Log() {       
    if (level == -1) {
        cerr << "ERROR: " << ss.str() << endl;
    } else if (level <= verbosity) {
        //time_t now = time(0);
        //cout << ctime(&now) ;
        
        if (level > 0) { cout << "DEBUG: "; }
        cout << ss.str() << endl;
    }
}
std::ostringstream& Log::log (int lvl) { level = lvl; return ss; }
std::ostringstream& Log::msg ()        { level = 0;   return ss; }
std::ostringstream& Log::err ()        { level = -1;  return ss; }

////////// IMAGE helpers  //////////////////////////////////////////////



// add B to A // modifies A
imglib::Image<float>& imgAdd(imglib::Image<float>& A, imglib::Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)+B(x,y,c);

    return A;
}

// subtract B from A // modifies A
imglib::Image<float>& imgSub(imglib::Image<float>& A, imglib::Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)-B(x,y,c);

    return A;
}

// add constant value to all pixels
imglib::Image<float>& imgAdd(imglib::Image<float>& A, rgb color) {
	for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) += color.getVec()[c];

    return A;
}


// multiply by a scalar // modifies A
imglib::Image<float>& imgMul(imglib::Image<float>& A, float scalar)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c) * scalar;

    return A;
}


// get max pixel value
float imgMax(imglib::Image<float>& A)
{
    double max=0;
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                if (A(x,y,c) > max) max = A(x,y,c);
                
    return max;
}


// get min pixel value
float imgMin(imglib::Image<float>& A)
{
    double min=0;
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                if (A(x,y,c) < min) min = A(x,y,c);
                
    return min;
}



// scale image so max value is 1 (over all channel at once)
imglib::Image<float>& imgScale(imglib::Image<float>& A)
{
    return imgMul (A, 1.0f / imgMax(A));
}


// 7x7 gauss kernel; sum is 140
const int gf7[7][7] = { { 1, 1, 2, 2, 2, 1, 1 },
                        { 1, 2, 2, 4, 2, 2, 1 }, 
                        { 2, 2, 4, 8, 4, 2, 2 },
                        { 2, 4, 8,16, 8, 4, 2 },
                        { 2, 2, 4, 8, 4, 2, 2 }, 
                        { 1, 2, 2, 4, 2, 2, 1 }, 
                        { 1, 1, 2, 2, 2, 1, 1 } };
                        
// simple 7x7 gauss filter; doues not handle boundary conditions
rgb sampleGauss7 (imglib::Image<float> image, int xpos, int ypos)
{
    assert (image.getNumChannels() == 3);
    assert ( (xpos-3 >= 0) && (xpos+3 < image.getWidth()) );
    assert ( (ypos-3 >= 0) && (ypos+3 < image.getHeight()) );
    vector<double> color;
    color.resize(3);
    
    for (int c=0; c<3; c++) {
        color[c]=0;
        for (int x=0; x<7; x++)
            for (int y=0; y<7; y++)
                color[c] += (double)gf7[x][y] * (double)image(xpos+x-3,ypos+y-3,c);
        color[c] /= 140.0;
    }
    return rgb(color);
}


#define M_SQRT2PI   2.50662827463100050241     // sqrt(2*PI)

// normal distribution
double normalDistribution (double sigma, double mu, double x)
{
    double expfrac = (x-mu)*(x-mu) / (2 * sigma*sigma);
    return exp(-expfrac) / (sigma * M_SQRT2PI);
}

//////////  COLOR and TONEMAPPING  /////////////////////////////////////


rgb mapGamma (rgb value, double gain, double lambda) {
    rgb result;
    result.r = gain * pow(value.r, lambda);
    result.g = gain * pow(value.g, lambda);
    result.b = gain * pow(value.b, lambda);
    return result;
}

// linear scale value v, so that whitepoint wp is mapped to 1.0 and bp is mapped to 0.0
double mapLinear (double val, double wp, double bp) {
    return 1/(wp-bp) * val + bp/(bp-wp);
}
rgb mapLinear (rgb val, rgb wp, rgb bp) {
    rgb result;
    result.r = mapLinear(val.r, wp.r, bp.r);
    result.g = mapLinear(val.g, wp.g, bp.g);
    result.b = mapLinear(val.b, wp.b, bp.b);
    return result;
}

// linear RGB to sRGB
rgb rgb2srgb ( rgb linear ) {
    rgb sRGB;
    sRGB.r = rgb2srgb_component ( linear.r );
    sRGB.g = rgb2srgb_component ( linear.g );
    sRGB.b = rgb2srgb_component ( linear.b );
    return sRGB;
}

double rgb2srgb_component ( double value ) {
    if (value <= 0.0031308) {
        return 12.92 * value;
    } else {
        return 1.055 * pow (value, (1.0/2.4) ) - 0.055;
    }
}

// uses sRGB primaries (http://www.brucelindbloom.com/Eqn_RGB_XYZ_Matrix.html)
Vector3d rgb2xyY (rgb val) {
    double X = (0.4124564*val.r + 0.3575761*val.g + 0.1804375*val.b);
    double Y = (0.2126729*val.r + 0.7151522*val.g + 0.0721750*val.b);
    double Z = (0.0193339*val.r + 0.1191920*val.g + 0.9503041*val.b);

    Vector3d res(1,1,0);
    if (X+Y+Z==0) return res;
    res[0] = X / ( X+Y+Z );
    res[1] = Y / ( X+Y+Z );
    res[2] = Y;
    return res;
}
    

// sRGB to linear RGB
rgb srgb2rgb ( rgb sRGB ) {
    rgb linear;
    linear.r = srgb2rgb_component ( sRGB.r );
    linear.g = srgb2rgb_component ( sRGB.g );
    linear.b = srgb2rgb_component ( sRGB.b );
    return linear;
}

double srgb2rgb_component ( double value ) {
    if (value <= 0.04045) {
        return value / 12.92;
    } else {
        return pow ( (value+0.055)/1.055, 2.4 );
    }
}

double clamp (double val) {
    if (val < 0.0) val = 0.0;
    if (val > 1.0) val = 1.0;
    return val;
}
rgb clamp (rgb val) {
    if (val.r < 0.0) val.r = 0.0;
    if (val.r > 1.0) val.r = 1.0;
    if (val.g < 0.0) val.g = 0.0;
    if (val.g > 1.0) val.g = 1.0;
    if (val.b < 0.0) val.b = 0.0;
    if (val.b > 1.0) val.b = 1.0;
    return val;
}


//////////  GEOMETRIC   ///////////////////////////////////////////////

circle getCircumCircle (Vector2d p1, Vector2d p2, Vector2d p3) {
    circle area;
    
    // middle of two edges
    Vector2d midA = p1.cast<double>() + (p2-p1).cast<double>()/2.0f;
    Vector2d midB = p2.cast<double>() + (p3-p2).cast<double>()/2.0f;
    // vector from vertices to middle of edges
    Vector2d dirA = midA-p1.cast<double>();
    Vector2d dirB = midB-p2.cast<double>();
     // perpendicular vectors to dirA/B
    Vector2d perA = midA+Vector2d(dirA[1],-dirA[0]);
    Vector2d perB = midB+Vector2d(dirB[1],-dirB[0]);

    double x1=perA[0], y1=perA[1], x2=midA[0], y2=midA[1];
    double x3=perB[0], y3=perB[1], x4=midB[0], y4=midB[1];
    area.x = (int)roundf((x1*y2-y1*x2)*(x3-x4)-(x1-x2)*(x3*y4-y3*x4) )/( (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4) );
    area.y = (int)roundf((x1*y2-y1*x2)*(y3-y4)-(y1-y2)*(x3*y4-y3*x4) )/( (x1-x2)*(y3-y4) - (y1-y2)*(x3-x4) );
    area.r = (int)roundf(sqrt( (p1[0]-area.x)*(p1[0]-area.x)+ (p1[1]-area.y)*(p1[1]-area.y) ));
    
    return area;
}

// does an uniform sampling of the upper unit hemisphere
// returns numSamples^2 directions as spherical coordinates
directions sampleHemisphere (int numSamplesH, int numSamplesA)
{
    directions dirs;
    
    const double stepsizeH = 1.0 / (double)(numSamplesH);
    const double stepsizeA = M_PI * 2 / (double)(numSamplesA);
    
    Vector3d cart;
    for (int h=0; h<numSamplesH; ++h) {        // height (vertical coordinate)
        for (int a=0; a<numSamplesA; ++a) {    // azimut (position on circle)
            double height = (h) * stepsizeH;
            //double azimuth = (h%2==1)? (a*stepsizeA) : (a+0.5)*stepsizeA;
            double azimuth = (a*stepsizeA);
            double radius = sqrt (1.0 - height*height);
            cart[0] = radius * cos (azimuth);    // x  
            cart[1] = height;                    // y
            cart[2] = radius * sin (azimuth);    // z
            dirs.push_back ( cart );
        }
    }
    
    return dirs;
}

// does an uniform sampling of the unit sphere
// returns numSamples^2 directions as spherical coordinates
// alpha describes the angle below the horizon that limits the sampling (no samples below that angle are taken)
directions sampleSphere (int numSamplesH, int numSamplesA, double horizonAngle) {
    
    directions dirs;
    
    
    // highest and lowest y coordinate
    double yMax = 1.0;
    double yMin = -tan(horizonAngle);
    
    
    
    const double stepsizeH = (yMax+(-yMin)) / (double)(numSamplesH);
    const double stepsizeA = M_PI * 2 / (double)(numSamplesA);
    
    Vector3d cart;
    for (double y=yMin; y<yMax; y += stepsizeH ) { // height (vertical coordinate)
        for (int a=0; a<numSamplesA; ++a) {     // azimut (position on circle)
            double azimuth = (a*stepsizeA);
            double radius = sqrt (1.0 - y*y);
            cart[0] = radius * cos (azimuth);    // x  
            cart[1] = y;                         // y
            cart[2] = radius * sin (azimuth);    // z
            dirs.push_back ( cart );
        }
    }
    
    return dirs;
    
}

// do a random direction sampling w/ gradually decreasing min-distance to previous chosen directions
directions sampleUniform (int numSamples, double horizonAngle)
{   
    Log().msg() << "doing a random sampling of " << numSamples << " with decreasing mininmal distance";
    
    srand(23);
    directions dirs(0);
    
    const int maxFails = 10000;
    int numFails=0;
    double minDist = M_PI / 2;
    
    while (dirs.size() < numSamples) {
        
        // random direction within range
        double p = M_PI * (2*(double)rand()/RAND_MAX - 1);              // phi  (-pi..pi)
        double t = (M_PI_2+horizonAngle) * (double)rand() / RAND_MAX;   // theta (0..pi)
        Vector3d randomDir = spherical2cartesian(Vector2d(t,p));

        bool intersect = false;      
        
        // check distance to previous directions
        for (int d=0; d<dirs.size(); d++) {
            double cos_alpha =  randomDir.dot(dirs[d]);
            
            // due to rounding errors:
            if (cos_alpha > 1) { cos_alpha=1; }
            
            if (acos(cos_alpha) < minDist) {
                intersect=true;
                break;
            }
        }
        
        // repeat or reduce min distance 
        if (intersect) {
            numFails++;
            if (numFails >= maxFails) {
                 minDist *= 0.99;
                 numFails=0;
             }
        
        // direction was found
        } else {
            dirs.push_back(randomDir);
            Log().log(1) << "got " << dirs.size() << " minDist=" << minDist;
        }
        
    }
    
    return dirs;
}

directions samplesFromFile ( string path, int numVecs, double horizonAngle) {
    directions allDirs = loadVectors3d (path, numVecs);
    directions usedDirs;
    
    // normalize directions and remove the ones outside the sampling range
    for (int d=0; d<allDirs.size(); d++) {
        allDirs[d] = allDirs[d].normalized();
        if (allDirs[d](1) >= -sin(horizonAngle)) {
            usedDirs.push_back(allDirs[d]);
        }
    }
    return usedDirs;
}

directions loadVectors3d ( string path, int numVecs)
{
    Log().msg() << "loading " << numVecs << " vectors from " << path;
    directions dirs;
    ifstream in;
    in.open(path.c_str(), ios::in);
    for (int i=0; i<numVecs; ++i) {
        Vector3d dir(0,0,0);
        in >> dir[0]; in >> dir[1]; in >> dir[2];
        dirs.push_back(dir);
    }    
    in.close();
    
    return dirs;
}


int findNearestNeighbor(Vector3d vec, directions candidates)
{  
    double minDist = 1e10;
    int cand = -1;
    for (int d=0; d<candidates.size(); d++) {
        double alpha =  acos(vec.dot(candidates[d]));
        if (alpha < minDist) {
            minDist = alpha;
            cand = d;
        }
        
    }
    return cand;
}

Vector2d cartesian2spherical (Vector3d cartesian)
{
    cartesian = cartesian.normalized();
    Vector2d spherical;
    //double radius = sqrt( cartesian[0]*cartesian[0] + cartesian[1]*cartesian[1] + cartesian[2]*cartesian[2]);
    double radius = 1.0; // has to be one
    spherical[0] = acos(cartesian[2] / radius);          // theta
    spherical[1] = atan2(cartesian[1], cartesian[0]);    // phi
    return spherical;
}

// vector2d: (theta, phi)
Vector3d spherical2cartesian (Vector2d spherical)
{
    Vector3d cartesian;
    double radius = 1.0;
    cartesian[0] = radius * sin(spherical[0]) * cos(spherical[1]);   // x
    cartesian[1] = radius * cos(spherical[0]);                       // y
    cartesian[2] = radius * sin(spherical[0]) * sin(spherical[1]);   // z
    return cartesian.normalized();
}

// gives the angle between two spherical coordinates (on unit sphere)
double angle (Vector2d p1, Vector2d p2)
{
    Vector3d c1 = spherical2cartesian(p1);
    Vector3d c2 = spherical2cartesian(p2);
        
    // angle between vectors
    return acos(c1.dot(c2)); 
}

