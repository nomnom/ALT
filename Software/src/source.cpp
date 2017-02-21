/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * The Source class acquires, linearizes and returns images. Supports threading.
 *
 */

#include "source.h"


Source::Source() : running(false), locked(false), exposure(1.0), whitepoint(rgb(1,1,1)), responseType(RESPONSE_LINEAR), responseSize(0) {}
Source::~Source()
{    
 for (int c=0; c<3; c++) 
    if (responseCurve[c] != NULL) { delete[] responseCurve[c];    }
};

void Source::stop() {
    running = false;
}

void Source::start() {
    running = true;
    pthread_t thread;
    pthread_create (&thread, NULL, start_thread, this); 
}

void* Source::start_thread (void *ptr)
{ 
    // get pointer to class
    Source *tc = reinterpret_cast<Source*>(ptr);
    
    struct timespec update_cycle = { 0, 1e9/tc->updateRate };
    struct timespec fast_cycle = { 0, 1e9/tc->updateRate/10 };
    
    while (tc->running) {
         // wait until we can write
        while (tc->locked) { nanosleep(&fast_cycle, NULL); }
        if (tc->hasNewData()) {
            tc->locked = true;
            tc->acquire();
            tc->locked = false;
        }
        nanosleep(&update_cycle, NULL);
    }
    return 0;
}


// returns linearized image
imglib::Image<float>& Source::getImage ()
{ 
    // wait until unlocked
    struct timespec fast_cycle = { 0, 1e9/updateRate/10 };
    while (locked) {  nanosleep(&fast_cycle, NULL); }
    locked = true;
    imageCopy = imglib::Image<float> (imageBuffer);
    locked = false;
    return linearize(imageCopy);
}


void Source::setExposure(double exp) { exposure = exp; }
void Source::setWhitepoint(rgb wp) { whitepoint = wp; }
void Source::setResponseCurve(string reponseStr) {

    // load response curve
    responseSize = 0;
    // explicit nulling
    responseCurve[0]=NULL;
    responseCurve[1]=NULL;
    responseCurve[2]=NULL;
    if (reponseStr.compare("linear") == 0) {
        responseType=RESPONSE_LINEAR;
    } else if (reponseStr.compare("srgb") == 0) {
        responseType=RESPONSE_SRGB;
    } else {
        // load response curve if file exists
        ifstream file (reponseStr.c_str());
        if (file) {
            responseType = RESPONSE_FILE;
            loadResponseCurve(reponseStr);
        } else {
            responseType = RESPONSE_LINEAR;
        }
    }
        
    Log().log(1) << "response curve type is " << responseType;
    Log().log(1) << "applied white point is " << whitepoint.r << " " << whitepoint.g << " " << whitepoint.b ;
    

    
}
       

/**
 * Normalizes the response curve so the image values fits in the range (0:1).
 * The largest value of the three channels of the response curve is mapped to 1.0.
 * The largest value at index 0 is mapped to zero, so we have no positive offset. 
 * All three channels are scaled with the same value to preserve the relative relation.
 */
void Source::normalizeResponse () {
	double maxVal = max(max(responseCurve[0][responseSize-1], responseCurve[1][responseSize-1]), responseCurve[2][responseSize-1]);
    double minVal = max(max(responseCurve[0][0], responseCurve[1][0]), responseCurve[2][0]);
	for (int c=0; c<3; c++) {
		for (int i=0; i<responseSize; i++) {
			responseCurve[c][i] = clamp(mapLinear(responseCurve[c][i], maxVal, minVal));
		}
	}
}

/**
 * loads an three-channel response curve (either .m format created with hdrcalibrate, or a white-space separated three-column list)
 */
void Source::loadResponseCurve (string filename) {

    ifstream in;
    in.open(filename.c_str(), ios::in);
    vector<rgb> values; 

    // fileextension determines format
    string suffix = ".m";
    if (std::equal(filename.begin() + filename.size() - 2, filename.end(), suffix.begin() ) ) {
        
        // load .m format
        int color=-1; // current color
        int numRows=0;
        string tmp;
        while (!in.eof()) {
            getline (in,tmp);
            if (tmp.c_str()[0] == '#') {
                if (tmp.length() == 35)  {
                    if (tmp.substr(25,10) == "channel IR") { color = 0; }
                    else if (tmp.substr(25,10) == "channel IG") { color = 1; }
                    else if (tmp.substr(25,10) == "channel IB") { color = 2; }
                } else if (tmp.length() > 7) {
                    
                    if (tmp.substr(0,7) == "# rows:") { 
                        stringstream ss; ss << tmp.substr(8); ss >> numRows;
                        
                        if (values.size() < numRows) {
                            for (int i=values.size(); i<numRows; i++) {
                                values.push_back(rgb(0,0,0));
                            }
                        }
                    }
                }
                
            } else if (color >= 0 && numRows > 0) {
                double logVal, val;
                int index;
                for (int i=0; i<numRows; i++) {
                    
                    stringstream ss (tmp);
                    
                    ss >> logVal; ss >> index; ss >> val;
                    
                    switch (color) {
                        case 0: values[index].r = val; break;
                        case 1: values[index].g = val; break;
                        case 2: values[index].b = val; break;
                        default: break;
                    }
                    
                    getline(in, tmp);
                }
                
                
                if (color == 2) break;
                
            }
            
        }
        responseSize = numRows;
    } else {
        
        // assume values in three columns
        rgb tmp;
        while (in >> tmp.r && in >> tmp.g && in >> tmp.b) {
            values.push_back (tmp);
        }
        responseSize = values.size(); 
        
    }
    
    
    in.close();
    
    responseCurve[0] = new double [responseSize];
    responseCurve[1] = new double [responseSize];
    responseCurve[2] = new double [responseSize];
    
    for (int i=0; i<responseSize; i++) {
        responseCurve[0][i] = values[i].r;
        responseCurve[1][i] = values[i].g;
        responseCurve[2][i] = values[i].b;
        //cout << ">>>>" << i << " : " << values[i].r << " " <<  values[i].g << " " <<  values[i].b << endl;
    }
    
   normalizeResponse();
   
   Log().msg() << "loaded response curve of size " << responseSize  << " from " << filename; 
}



#define FLOOR_NOISE_THRESHOLD  (0.000)
#define HIGHLIGHT_TRESHOLD (0.9)

/**
 * Linearizes a single value using the supplied response curve and maps the white point.
 */
double Source::linearize(double value, int channel) {
    assert (value >= 0.0);
    assert (value <= 1.0);
    double result;
    switch (responseType) {
        case RESPONSE_LINEAR: result = value; break;
        case RESPONSE_SRGB:    result = srgb2rgb_component(value); break;
        case RESPONSE_FILE:  
                
            if (value < FLOOR_NOISE_THRESHOLD) value=0;
            if (value > HIGHLIGHT_TRESHOLD) value = HIGHLIGHT_TRESHOLD;  // clips the canon response curve to the linear part
            int idx=int(round(value*double(responseSize-1)));
            assert (idx>=0);
            assert (idx < responseSize);
            result = responseCurve[channel][ idx ];
            break;
    }
    result  =  mapLinear(result, whitepoint.getVec()[channel], 0) / exposure;
    return result;
}

/**
 * Linearizes an image using the supplied response curve and maps the white point.
 */
imglib::Image<float>& Source::linearize(imglib::Image<float>& A) {

    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<3; c++)
                A(x,y,c) = linearize(A(x,y,c),c);
    
    return A;
}




