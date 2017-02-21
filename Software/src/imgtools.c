/*
 * simple image tools
 *
 * @author Manuel Jerger
 *
 */

#include "image.h"
#include <string>
#include <getopt.h>  
#include <stdlib.h> 
#include <algorithm>

using namespace std;
using namespace imglib;


struct circle { circle() : x(0), y(0), r(0) {}; double x; double y; double r; };
circle circular;
    
struct rgb { rgb() : r(0), g(0), b(0) {}; float r; float g; float b; };

enum MODE { NONE, ADD, SUB, MUL, MIN, MAX, MEDIAN, THRESH, AVG, LSD, DIFF, CCROP }; 
int mode = NONE;

string outfile;
vector<Image<float> > infiles;
float minthresh=0;
int size;
bool normalize=false;
double scale=0;
bool whitepoint=false;
rgb wp;
bool mapInput=false;

rgb avg;
rgb lsd;

string responseFile;
int responseSize;
double* responseCurve [3];
    
// add B to A // modifies A
Image<float>& imgAdd(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)+B(x,y,c);

    return A;
}

// subtract B from A // modifies A
Image<float>& imgSub(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)-B(x,y,c);

    return A;
}


// multiply two images // modifies A
Image<float>& imgMul(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c)*B(x,y,c);
    return A;
}


// multiply by a scalar // modifies A
Image<float>& imgMulScalar(Image<float>& A, float scalar)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                A(x,y,c) = A(x,y,c) * scalar;

    return A;
}



// scale max value to 1.0; preserves color // modifies A
Image<float>& imgScaleMax(Image<float>& A)
{
    double maxVal = 0;
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++)
                maxVal = A(x,y,c) > maxVal ? A(x,y,c) : maxVal;
                
    imgMulScalar(A, 1.0/maxVal);
    return A;
}




// clamp between 0 .. 1
Image<float>& clamp(Image<float>& A)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<A.getNumChannels(); c++ ) {
                if (A(x,y,c) < 0) A(x,y,c)=0; 
                if (A(x,y,c) > 1) A(x,y,c)=1; 
            }
    return A;
}


//(2n+1)x(2n+1) min filter
Image<float>& minmax(Image<float>& A, int n, bool usemax)
{
    Image<float> O (A);
    float val;
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
            {
                val = usemax ? 0 : 1e20;
                for (int dx=-n; dx<=n; dx++)
                    for (int dy=-n; dy<=n; dy++)
                        if ( (x+dx >= 0) && (x+dx < A.getWidth()) && (y+dy >= 0) && (y+dy < A.getHeight()) )
                            if (usemax) { val = (O(x+dx,y+dy,c)>val) ? O(x+dx,y+dy,c) : val; }
                            else { val = (O(x+dx,y+dy,c)<val) ? O(x+dx,y+dy,c) : val; }
                            
                A(x,y,c) = val;
            }
    return A;
}



//(2n+1) x (2n+1) median filter
Image<float>& median(Image<float>& A, int n)
{
    Image<float> O (A);
    
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
            {
                vector<float> vals;
                for (int dx=-n; dx<=n; dx++)
                    for (int dy=-n; dy<=n; dy++)
                        if ( (x+dx >= 0) && (x+dx < A.getWidth()) && (y+dy >= 0) && (y+dy < A.getHeight()) )
                            vals.push_back( O(x+dx,y+dy,c) );
                
                // sort vector and pic center element
                sort(vals.begin(), vals.end());
                if (vals.size() % 2 == 0) {
                    A(x,y,c) = vals[vals.size() / 2 - 1] / 2 + vals[vals.size() / 2] / 2;
                } else {
                    A(x,y,c) = vals[vals.size()/2];
                }
            }
    return A;
}

//lower threshold
Image<float>& threshold (Image<float>& A, float minval)
{
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
                A(x,y,c) = (A(x,y,c)<minval) ? 0 : A(x,y,c);
    return A;
}

// average circular area
rgb avgCircularArea (Image<float> A, circle area)
{
    rgb sum;
    int numSamples=0;

    for (int y=area.y-area.r; y<area.y+area.r; ++y)
        for (int x=area.x-area.r; x<area.x+area.r; ++x)
            if ((area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) < area.r*area.r) {
                sum.r += A(x,y,0); sum.g += A(x,y,1); sum.b += A(x,y,2);
                numSamples ++;
            }
            
    sum.r /= (float)numSamples;
    sum.g /= (float)numSamples;
    sum.b /= (float)numSamples;
    return sum;
}

// circular crop (zero other pixels)
Image<float>& circularCrop (Image<float>& A, circle area)
{
    for (int y=0; y<A.getHeight(); ++y)
        for (int x=0; x<A.getWidth(); ++x)
            if ((area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) > area.r*area.r)
                for (int c=0; c<3; c++) { A(x,y,c) = 0; }
    return A;
}


// sum of square differences // modifies A and sets per-pixel-residues
rgb sumLSD (Image<float>& A, Image<float> B, circle area)
{
    rgb sum;
    int numSamples = 0;
    
    // over all pixels
    if (circular.x == 0) {
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++) {
                for (int c=0; c<3; c++) 
                    A(x,y,c) = (A(x,y,c) - B(x,y,c)) * (A(x,y,c) - B(x,y,c));
                    
                sum.r += A(x,y,0);
                sum.g += A(x,y,1);
                sum.b += A(x,y,2);
                numSamples++;
            }
            
    // circular area only
    } else {
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
                if ((area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) <= area.r*area.r) {
                    for (int c=0; c<3; c++) 
                        A(x,y,c) = (A(x,y,c) - B(x,y,c)) * (A(x,y,c) - B(x,y,c));
                        
                    sum.r += A(x,y,0);
                    sum.g += A(x,y,1);
                    sum.b += A(x,y,2);
                    numSamples++;
                }
    }        
        
    sum.r /= (float)numSamples;
    sum.g /= (float)numSamples;
    sum.b /= (float)numSamples;
    return sum;
}



Image<float>& diff(Image<float>& A, Image<float>& B)
{
    for (int x=0; x<A.getWidth(); x++)
        for (int y=0; y<A.getHeight(); y++)
            for (int c=0; c<3; c++) 
                A(x,y,c) = abs(A(x,y,c) - B(x,y,c));
    return A;
}

#define FLOOR_NOISE_THRESHOLD  (0.000)
#define HIGHLIGHT_TRESHOLD (0.9)

// apply response curve to value
double linearize(double value, int channel) {
    assert (value >= 0.0);
    assert (value <= 1.0);

    if (value < FLOOR_NOISE_THRESHOLD) value=0;
    if (value > HIGHLIGHT_TRESHOLD) value = HIGHLIGHT_TRESHOLD;  // clips the canon response curve to the linear part

    double result;
    int idx=int(round(value*double(responseSize-1)));
    assert (idx>=0);
    assert (idx < responseSize);
    result = responseCurve[channel][ idx ];
    
    return result;
}

//map with response curve
Image<float>& map (Image<float>& A)
{
    for (int c=0; c<A.getNumChannels(); c++ )
        for (int x=0; x<A.getWidth(); x++)
            for (int y=0; y<A.getHeight(); y++)
                A(x,y,c) = linearize(A(x,y,c),c);
    return A;
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

Image<float> mapImgLinear ( Image<float> A, rgb wp)
{
   for (int x=0; x<A.getWidth(); x++)
      for (int y=0; y<A.getHeight(); y++) {
         A(x,y,0) = mapLinear(A(x,y,0), wp.r, 0);
         A(x,y,1) = mapLinear(A(x,y,1), wp.g, 0);
         A(x,y,2) = mapLinear(A(x,y,2), wp.b, 0);
      }

   return A;
}

// loads rgb response curve (either .m format created with hdrcalibrate, or a three-column list w/o indices)
void loadResponseCurve (string filename)
{
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
                                values.push_back(rgb());
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

    // fits image range of response curve to  (0:1);
	double maxVal = max(max(responseCurve[0][responseSize-1], responseCurve[1][responseSize-1]), responseCurve[2][responseSize-1]);
	double minVal = max(max(responseCurve[0][0], responseCurve[1][0]), responseCurve[2][0]);
	for (int c=0; c<3; c++) {
		for (int i=0; i<responseSize; i++) {
			responseCurve[c][i] = clamp(mapLinear(responseCurve[c][i], maxVal, minVal));
		}
	}
   cout << "loaded response curve of size " << responseSize  << " from " << filename << endl;
}

////////// CLI parsing  ////////////////////////////////////////////////


void parseArgs(int argc, char *argv[])
{
    int option_index = 0, c=0;
    while ( ( c = getopt (argc, argv, "i:o:t:asm:n:x:v:lr:pM:N:dc:w:L:")) != -1)
    {
	string tmp;
        stringstream arg;
        if (c) arg << (optarg);
        switch (c) {
           case 'i': arg >> tmp; cout << "loading " << tmp << endl; infiles.push_back(Image<float>(tmp));break;
	   case 'o': arg >> outfile; break;
	   case 'a': mode=ADD; break;
	   case 's': mode=SUB; break;
	   case 'p': mode=MUL; break;
       case 'n':  arg >> scale; normalize=true; break;
       case 'w':  arg >> wp.r; arg >> wp.g; arg >> wp.b; whitepoint=true; break;
       case 'N': mode=MIN; arg >> size; break;
       case 'M': mode=MAX; arg >> size; break;
       case 'm': mode=MEDIAN; arg >> size; break;
       case 't': mode=THRESH; arg >> minthresh; break;
       case 'v': mode=AVG; arg >> circular.x; arg >> circular.y; arg >> circular.r; break;
       case 'l': mode=LSD; break;
       case 'L': mode=LSD;  arg >> circular.x; arg >> circular.y; arg >> circular.r;   break;
       case 'r': mapInput=true; arg >> responseFile; loadResponseCurve(responseFile); break;
       case 'd': mode=DIFF;  break;
       case 'c': mode=CCROP; arg >> circular.x; arg >> circular.y; arg >> circular.r; break;
       
        }
    }
    
}


////////// MAIN  ///////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    // process commandline
    parseArgs(argc, argv);
    
    // map input images
    
    if (mapInput) {
        cout << "mapping images via response curve" << endl;
        for (int i=0; i<infiles.size(); i++) {
            infiles[i] = map(infiles[i]);
        }
    }    
    if (whitepoint) {
        cout << "correcting whitepoint ("<<wp.r<<" "<<wp.g<<" "<<wp.b<<")";
        for (int i=0; i<infiles.size(); i++) {
            infiles[i] = mapImgLinear(infiles[i], wp);
        }
    }      
    
    Image<float> result (infiles[0]);
    switch (mode) {
        case ADD: cout << "adding " << infiles.size() << " images" << endl; 
                  for (int i=1; i<infiles.size(); ++i) { imgAdd(result, infiles[i]); }  
                  break;
        case SUB: cout << "subtracting " << (infiles.size()-1) << " images from the first" << endl;
                  for (int i=1; i<infiles.size(); ++i) { imgSub(result, infiles[i]); }
                  break;
        case MUL: cout << "multiplying " << (infiles.size()) << " images" << endl;
                  for (int i=1; i<infiles.size(); ++i) { imgMul(result, infiles[i]); }
                  break;
        case MIN: cout << "running "<<2*size+1<<"x"<<2*size+1<<" min filter on first image" << endl;
                   result = minmax(result,size,false);
                   break;
        case MAX: cout << "running "<<2*size+1<<"x"<<2*size+1<<" max filter on first image" << endl;
                   result = minmax(result,size,true);
                   break;
        case MEDIAN: cout << "running "<<2*size+1<<"x"<<2*size+1<<" median filter on first image" << endl;
                   result = median(result,size);
                   break;
        case THRESH: cout << "tresholding image with " << minthresh << endl;
                     threshold (result, minthresh);
                     break;
        case AVG: cout << "averaging values over area" << endl;
                    avg = avgCircularArea(result, circular);
                    cout << "average r=" << avg.r << " g=" << avg.g << " b=" << avg.b << endl;
                    break;
        case LSD: cout << "calculating least square difference on pixels of first two images" << endl;
                    assert (infiles.size() >= 2);
                    lsd = sumLSD(result, infiles[1], circular);
                    cout << "LSD is " << (lsd.r+lsd.g+lsd.b)/3.0 << " (r=" << lsd.r << " g=" << lsd.g << " b=" << lsd.b << ")" << endl;
                    break;
        case DIFF: cout << "diffing first two input images" << endl;
                    assert (infiles.size() >= 2);
                    result = diff (infiles[0], infiles[1]);
                    break;
        
        case CCROP: cout << "performing circular crop (" << circular.x << " " << circular.y << " " << circular.r << ") on first iamge" << endl;
                    circularCrop(result, circular);
                    break;
        default: cout << "no mode specified" << endl; break;
    }
   
    if (not outfile.empty()) {
        if (normalize) {
            cout << "rescaling result";
            if (scale == 0) {
                cout << " automatically" << endl;
                imgScaleMax(result);
            } else {
                cout << " with " << scale << endl;
                imgMulScalar(result,scale);
            }
        } else {
            cout << "clamping result" << endl;
            clamp(result);
        }
        cout << "writing result to " << outfile << endl;
        result.save(outfile);
    }
    return 0;
}





