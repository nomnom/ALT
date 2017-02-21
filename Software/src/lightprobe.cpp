/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Our light probe model based on reflection mapping using a single sphere and a pinhole camera model.
 * Also supports images created with the parametrization from Debevec  http://www.pauldebevec.com/Probes/
 *
 */

#include "lightprobe.h"

using namespace std;


/**
 *  Set up light probe model from a given model config and sampling config.
 */
Lightprobe:: Lightprobe (Source* s, params c, samplingParams sampling) : 
    source(s), config(c), samplingConfig (sampling)
{  
    init(); 
}

/**
 *  Set up light probe model using a given sampling configuration. Loads model config from file.
 */
Lightprobe::Lightprobe (Source* s, string configFile, samplingParams sampling) : 
    source(s), samplingConfig (sampling)
{
    config.load(configFile);
    init();
}


/**
 *  Set up light probe model using a given sampling configuration and sampling directions. Loads model config from file.
 */
Lightprobe::Lightprobe (Source* s, params c, samplingParams sampling, directions samplingDirs) : 
    source(s), config(c), samplingConfig (sampling), samplingDirs(samplingDirs)
{
    init();
}

Lightprobe::~Lightprobe () { }


/**
 * Initializes the light probe model.
 */
void Lightprobe::init()
{
    Log().log(1) << "initializing Lightprobe with:";
    Log().log(1) << "  sphereCircle = \"" << config.sphereCircle.x << " " << config.sphereCircle.y << " " << config.sphereCircle.r << "\"" ;
    Log().log(1) << "  dist. lense to sphere center = " << config.camDistance << " cm";
    Log().log(1) << "  sphereRadius = " << config.sphereRadius << " cm";
    Log().log(1) << "  probe rotation x=" << config.rotation(0) << ", y=" << config.rotation(1) << ", z=" << config.rotation(2) << " rad";
    Log().log(1) << "  lowest angle below horizon = " << config.horizonAngle << " rad";
    Log().log(1) << "  probe whitepoint  r=" << config.whitepoint.r << "  g=" << config.whitepoint.g << "  b=" << config.whitepoint.b;
    Log().log(1) << "  response curve is " << config.responseCurve;
    Log().log(1) << "  mask image is " << config.maskFile;
    
    
    // load mask image if set
    ifstream file (config.maskFile.c_str());
    if (file) {
        maskImage = imglib::Image<float>(config.maskFile);
        file.close();
        
    } else {
        maskImage = imglib::Image<float>(source->getWidth(), source->getHeight(), 1);
        for (int x=0; x<source->getWidth(); ++x) {
            for (int y=0; y<source->getHeight(); ++y) {
                maskImage(x,y,0) = 1.0;
            }
        }
        config.maskFile = "not set";
    }
    
    // calculate image plane shift (see documentation for more infos)
    planeShift = config.sphereRadius*config.sphereRadius / config.camDistance;  
    
    precalculateDirectionPixelData();
    if (samplingDirs.size() == 0 ) { 
        precalculateSamplingDirections();
    } else if (samplingConfig.samplingMode == samplingParams::ALLPIXELS) {
        precalculateSamplingAllPixels();
    }
    precalculateSamplingStructure();
    
    // propagate response curve to image source
    source->setWhitepoint(config.whitepoint);
    source->setResponseCurve(config.responseCurve);
    source->setExposure(config.exposure);
    

}

Lightprobe::params Lightprobe::getConfig() { return config; };
Lightprobe::samplingParams Lightprobe::getSamplingConfig() { return samplingConfig; }

Source* Lightprobe::getSource() {  return source; }

void Lightprobe::acquire ()
{ 
    source->acquire();
}

imglib::Image<float>& Lightprobe::getImage()
{ 
    return source->getImage(); 
}

bool Lightprobe::hasNewData ()
{ 
    return source->hasNewData();
}

/**
 * Loads model config from a file.
 */ 
void Lightprobe::params::load (string file) {
    ifstream in;
    in.open(file.c_str(), ios::in);
    string tmp;
    in >> tmp;
    if (tmp != "config_lightprobe1") { 
        Log().err() << "config file " << file << " is not compatible with this lightprobe"; 
    } else {
        in >> sphereRadius;
        in >> sphereCircle.x;  in >> sphereCircle.y; in >> sphereCircle.r;
        in >> camDistance;
        in >> rotation[0]; rotation[0] *= M_PI / 180.0; 
        in >> rotation[1]; rotation[1] *= M_PI / 180.0; 
        in >> rotation[2]; rotation[2] *= M_PI / 180.0; 
        in >> exposure; 
        in >> gamma;
        in >> whitepoint.r; in >> whitepoint.g; in >> whitepoint.b;
        in >> responseCurve;
        in >> maskFile;
        Log().log(0) << "successfully loaded lightprobe config from " << file  << endl;
    }
    in.close();
}

/**
 * Saves model config to a file.
 */
void Lightprobe::params::save (string file) {
    ofstream out;
    out.open(file.c_str(), ios::out);
    out << "config_lightprobe1" << endl;
    out << sphereRadius << endl;
    out << sphereCircle.x << " " << sphereCircle.y << " " << sphereCircle.r << endl;
    out << camDistance << endl;
    out << rotation[0]*180.0 / M_PI  << " " << rotation[1]*180.0 / M_PI  << " " << rotation[2]*180.0 / M_PI  << endl;
    out << exposure << endl;
    out << gamma << endl;
    out << whitepoint.r << " " << whitepoint.g << " " << whitepoint.b << endl;
    out << responseCurve << endl;
    out << maskFile << endl;
    out.close();
    Log().log(0) << "saved lightprobe config to " << file  << endl;
}
    



/**
 * Calculates the impact of a lamp: Acquires an image from the source, performs downsampling and returns the sampled values.
 */
vector<rgb> Lightprobe::getImpact ()
{
    return getImpact(source->getImage());
}

/**
 * Set new value for rotation around y axis (on planar plane) and recalculate sampling datastructures
 */
void Lightprobe::setRotationY ( double rad ) {
    config.rotation(1) = rad;
    precalculateDirectionPixelData();
    //precalculateSamplingDirections();
    precalculateSamplingStructure();
}

/**
 * Samples a light probe image.
 */
vector<rgb> Lightprobe::getImpact(imglib::Image<float>& img)
{
    Log().log(1) << "calculating lighting impact";
    
    double thresh = 1e-2;
    
    int cnt=0;
    int cntUsed=0;
    
    int numDirs = samplingDirs.size();
    vector<rgb> impacts;
    for (int i=0; i < numDirs; ++i ) {
                
        dirCone cone = samplingCones[i];
        
        double weightSum = 0;
        
        // sample the neighborhood of linearized values
        rgb color(0,0,0);
        for (int n=0; n < cone.pixels.size(); ++n) {

            color.r += cone.weights[n] * img(cone.pixels[n][0], cone.pixels[n][1], 0);
            color.g += cone.weights[n] * img(cone.pixels[n][0], cone.pixels[n][1], 1);
            color.b += cone.weights[n] * img(cone.pixels[n][0], cone.pixels[n][1], 2);
            
            weightSum += cone.weights[n];
            cnt++;

        }
        
        if (weightSum != 0) {
            color.r /= weightSum ;
            color.g /= weightSum ;
            color.b /= weightSum ;
        }
        
        
        if (color.r!=0 || color.g!=0 || color.b!=0) {
            cntUsed++;
        }
        
        impacts.push_back(color);
    }
    
    Log().log(1) << "sampled " << cnt << " times, " << cntUsed << " of " << numDirs << " are not zero";
    return impacts;
}

/**
 * Precalculates the direction of reflected light for all available, unmasked pixels within the sampling range.
 */
void Lightprobe::precalculateDirectionPixelData ()
{

    Log().msg() << "precalculating the corresponding direction for each pixel ..";

    allDirs.clear();
    allPixels.clear();
    
    int xmin = (int)round(config.sphereCircle.x - config.sphereCircle.r);
    int xmax = (int)round(config.sphereCircle.x + config.sphereCircle.r);
    int ymin = (int)round(config.sphereCircle.y - config.sphereCircle.r);
    int ymax = (int)round(config.sphereCircle.y + config.sphereCircle.r);
    
    int numAllDirs = 0;
    Vector3d dir; Vector2i pixel;
    for (int x=xmin; x<xmax; x++) { 
        for (int y=ymin; y<ymax; y++) {
            if ((config.sphereCircle.x-x)*(config.sphereCircle.x-x) + (config.sphereCircle.y-y)*(config.sphereCircle.y-y) < config.sphereCircle.r*config.sphereCircle.r) {
                pixel[0] = x; pixel[1] = y;
                dir = getDirectionFromPixel(pixel);
                if (dir[1] > -sin(config.horizonAngle)) { // use only directions 'above' the lowest angle
                    if (maskImage(x,y,0) > 0.0) {         // ignore masked pixels
                        allDirs.push_back ( getDirectionFromPixel(pixel));
                        allPixels.push_back (pixel);
                        numAllDirs++;
                    }
                
                }
            }
        }
    }
    Log().log(0) << " -> selected " << allDirs.size() << " pixels";
}



/**
 * Precalculates the sampling directions.
 */
void Lightprobe::precalculateSamplingDirections ()
{
 
    
    switch (samplingConfig.samplingMode) { 
        case samplingParams::UNIFORM_OLD: 
            samplingDirs = sampleSphere(samplingConfig.numSamplesH, samplingConfig.numSamplesA, config.horizonAngle);
            break;
        case samplingParams::UNIFORM:
            samplingDirs = sampleUniform(samplingConfig.numSamples, config.horizonAngle);
            break;
        case samplingParams::FROM_FILE:
            samplingDirs = samplesFromFile (samplingConfig.filename, samplingConfig.numSamples, config.horizonAngle);
            break;
        case samplingParams::ALLPIXELS:            
            precalculateSamplingAllPixels ();
            break;
        default: break;
    }
    
    // update number of samples
    samplingConfig.numSamples = samplingDirs.size();
}


/**
 * Generates the sampling data structure.
 */
void Lightprobe::precalculateSamplingStructure ()
{
    
    /*
    // for experiments: find and print mindistances
    double avgMinDist=0;
    for (int d=0; d<samplingDirs.size(); d++) {
        double minDist = 1e10; int cand = -1;
        for (int i=0; i<samplingDirs.size(); i++) {
            if (d==i) continue;
            double cos_alpha = samplingDirs[d].dot(samplingDirs[i]);
            if (cos_alpha > 1.0) cos_alpha = 1;
            double dist = acos(cos_alpha) ;    
            if (dist < minDist) { 
                minDist = dist;
                cand = i;
            }
        }
        avgMinDist += minDist;
        Log().log(1) << "closest for " << d << " is " << cand << " with " << minDist;
    }
    Log().log(1) << "average minDist is " << avgMinDist/(double)samplingDirs.size();
    */
    
    // apply kernel and precalculate sampling datastructures
    switch (samplingConfig.kernelMode) {
        case samplingParams::NONE: break;
        case samplingParams::NEIGHBOR: precalculateSamplingNearestNeighbors ();  break;
        case samplingParams::GAUSS:    precalculateSamplingCones ();             break;
        default: break;
    }
    
    // mark directions with few pixels as unused
    for (int d=0; d<samplingDirs.size(); d++) {
        if (samplingCones[d].pixels.size() < samplingConfig.minConeSize) {
            usedDirections[d] = false;
        }
    }
        
}
    


/** 
 * Generate sampling data structure for Gaussian sampling.
 */
void Lightprobe::precalculateSamplingCones ()
{
    Log().msg() << "precalculating sampling : fixed cone on " << samplingDirs.size() << " directions..";
    Log().log(1) << "sigma is " << samplingConfig.coneSigma << ", cone size is " << samplingConfig.coneSize << " rad";
    
    samplingCones.clear();
    usedDirections.clear();
    
    int coneSizeSum = 0;
    for (int l=0; l<samplingDirs.size(); l++) {
        Vector3d dir = samplingDirs[l];
        dirCone cone(dir);
        // find nearest neighbors within coneSize rad (angular space) and add them to the cone
        for (int n=0; n<allDirs.size(); n++) {
            double cos_alpha = dir.dot(allDirs[n]);
            if (cos_alpha > 1.0) cos_alpha = 1;     // clamp needed due to rounding errors
            double dist = acos(cos_alpha) ;       // TODO does this get optimized?
            if ( abs(dist) <= samplingConfig.coneSize*2 ) {
                if (samplingConfig.coneSigma > 0) { 
                    cone.add(allDirs[n], allPixels[n], normalDistribution (samplingConfig.coneSigma, 0 , (dist/samplingConfig.coneSize)));
                } else {
                    cone.add(allDirs[n], allPixels[n], 1.0);
                }
                coneSizeSum++;
            }
        }
        
        Log().log(2) << "cone contains " << cone.dirs.size() << " pixels";
        
        // save direction and neighborhood cone
        samplingCones.push_back(cone);
        usedDirections.push_back(true);
    }
    
    Log().log(1) << " -> total number of pixels to sample is " << coneSizeSum;
}



/**
 * 
 * Generates sampling datastructure for nearest-neighbor sampling.
 */
void Lightprobe::precalculateSamplingNearestNeighbors ()
{
    Log().msg() << "precalculating sampling : " << allDirs.size() << " nearest neighbors of " << samplingDirs.size() << " directions..";
    
    // create cones datastructure
    samplingCones.clear();
    usedDirections.clear();
    for (int l=0; l<samplingDirs.size(); l++) {
        samplingCones.push_back( dirCone(samplingDirs[l]) );
        usedDirections.push_back(true);
    }
    
    // for every pixel...
    for (int p=0; p<allDirs.size(); ++p) {
        
            
        // ..find the closest of the sampling directions and add pixel to the cone
        int closest=-1; double minDist=1e10;
        for (int l=0; l<samplingDirs.size(); l++) {
            double cos_alpha = samplingDirs[l].dot(allDirs[p]);
            if (cos_alpha > 1.0) cos_alpha = 1;
            double dist = acos(cos_alpha) ;       // TODO does this get optimized?
            if (dist < minDist) {
                minDist = dist;
                closest = l;
            }
        }
    
        if (closest > -1) {
            samplingCones[closest].add (allDirs[p], allPixels[p], 1);
        } else {
            Log().err() << "impossible error: no closest direction found";
        }
    }
    
    int coneSizeSum=0;
    for (int l=0; l<samplingDirs.size(); l++) {
        coneSizeSum += samplingCones[l].dirs.size();
        Log().log(2) << "cone contains " << samplingCones[l].dirs.size() << " pixels";
    }
}

/**
 * Generates sampling structure for all-pixel sampling (every pixel becomes one direction).
 */
void Lightprobe::precalculateSamplingAllPixels ()
{
    // create cones datastructure
    samplingDirs.clear();
    samplingCones.clear();
    usedDirections.clear();
    for (int l=0; l<allDirs.size(); l++) {
        samplingDirs.push_back( allDirs[l] );
        dirCone cone (allDirs[l]);
        cone.add (allDirs[l], allPixels[l], 1.0);
        samplingCones.push_back(cone);
        usedDirections.push_back(true);
    }
}


//////////////// geometric Lightprobe model   ///////////////////////////////


/**
 * Our light probe model. Calculates the reflected light direction from pixel coordinates.
 */
Vector3d Lightprobe::getDirectionFromPixel ( Vector2i pos )
{
    // hook: debevec mapping;
    // TODO create separate functions for sphere mapping
    if (config.type == 1) return getDirectionFromPixelDebevec(pos);
    
    // forward raycasting
    
    //Vector3d C(0,0,0);              // center of sphere at origin
    double scale = 1/config.sphereRadius;
    // scale model so the sphere has radius 1;
    Vector3d I(0, config.camDistance * scale , 0);      // position of pinhole in scaled model
    
    
    // image plane
    double pixelSize = 1/(config.sphereCircle.r) * (config.camDistance-planeShift)/config.camDistance;
    Vector3d forward = Vector3d(0, planeShift * scale, 0) - I;
    Vector3d up = Vector3d(pixelSize,0,0);
    Vector3d right = Vector3d(0,0,pixelSize);
    
    // move pixepos so the center is at the  origin
    pos = pos - Vector2i(config.sphereCircle.x, config.sphereCircle.y);
    
    Vector3d pixel = I + forward + right*((double)pos[0]) + up*((double)pos[1]);
    
    //////////////
    // EXPLANATION:
    // cast ray from I to pixel and intersect with sphere in point P
    // ray: P = I + t*d  where d=pixel-I
    // unit sphere:  (P-C).(P-C)=1
    // we have C=0:  (I + t*d).(I + t*d) = 1
    // expand:       (d.d)*t^2 + 2*(I.d)*t) + I.I-1 = 0
    // to quardr.:   a=d.d,  b=2I.d,  c=I.I-1
    // solve for t: t = ( -(2I.d) - sqrt((2I.d)^2 - 4*(d.d)*(I.I-1))  ) / 2a 
    
    Vector3d dir = pixel - I;  dir = dir.normalized();
    double A = dir.dot(dir);
    double B = 2.0 * I.dot(dir);
    double C = I.dot(I) - 1.0;
    double t = (-B - sqrt(B*B - 4*A*C)) / (2*A);
    
    // point on sphere (it's also the surface normale because unit sphere is at located at the origin)
    Vector3d P = I + dir * t;
    P=P.normalized();
    
    // get light direction from reflection
    Vector3d lightDir = -2*dir.dot(P)*P + dir;
    
    // rotate direction depending on cam angle
    lightDir = AngleAxis<double>(config.rotation(0), Vector3d(1,0,0)) * lightDir;
    lightDir = AngleAxis<double>(config.rotation(1), Vector3d(0,1,0)) * lightDir;  // rotate probe around y-axis (= on the horizontal plane)
    lightDir = AngleAxis<double>(config.rotation(2), Vector3d(0,0,1)) * lightDir;
    
    return  lightDir.normalized();
}



/**
 * Light probe model that use the Debevec parametrisation.
 */
Vector3d Lightprobe::getDirectionFromPixelDebevec ( Vector2i pos )
{
    
    // 1) map coordinates to -1..+1
    double xpos = 1.0 - 2*((double)pos[0]/(double)(source->getWidth()));
    double ypos = 1.0 - 2*((double)pos[1]/(double)(source->getHeight()));
    
    // calculate direction from position 
    double theta = atan2(ypos,xpos);
    double phi = M_PI * sqrt(xpos*xpos + ypos*ypos);
    Vector3d lightDir(0,0,-1);
    lightDir = AngleAxis<double>(phi, Vector3d(0,1,0)) * lightDir;
    lightDir = AngleAxis<double>(theta, Vector3d(0,0,-1)) * lightDir;
    
    // rotate direction depending on cam angle
    lightDir = AngleAxis<double>(config.rotation(0), Vector3d(1,0,0)) * lightDir;
    lightDir = AngleAxis<double>(config.rotation(1), Vector3d(0,1,0)) * lightDir; // rotate probe around y-axis (= on the horizontal plane)
    lightDir = AngleAxis<double>(config.rotation(2), Vector3d(0,0,1)) * lightDir;
    
    return lightDir.normalized();
}


