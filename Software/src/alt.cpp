/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 *  Ambient Light Transfer main program.
 *
 */

#include "alt.h"


using namespace std;


//
// program settings (set up while parsing command line arguments)
//

int progmode = TRANSFER;                    // what program mode to execute
int masterArg = 0;                          // last parset longopt
extern int verbosity;                       // 0: info   1: debug   2: verbose debug

// configuration of our objects
Calibrate::params   caliConfig;
Transfer::params    transferConfig;
Sticks::params      sticksConfig;
X11Source::params   x11sourceConfig;
ImageSource::params imagesourceConfig;
Lightprobe::params  probeConfig;
Lightprobe::samplingParams samplingConfig;

// setlamps has no class
vector<pair<int, double> > setLampsValues;  
rgb setLampsRGB;
string setLampsFile;

// setup of lamps
int numVirtualLamps=0;
bool useSticks = false;
int sourceType; 

// remembers if --probe or --sticks occured in arguments
bool probeArgs = false;
bool stickArgs = false;

static struct option longOptions[] =
{   
    // debug switches
    {"debug",     no_argument, &verbosity, 1},
    {"verbose",   no_argument, &verbosity, 2},
    
    // program modes
    {"transfer",    no_argument, &masterArg, TRANSFER},
    {"transfer_sampler",    no_argument, &masterArg, TRANSFER_SAMPLER},
    {"capture",     no_argument, &masterArg, CAPTURE},
    {"calibratelamps", no_argument, &masterArg, CALIBRATE_LAMPS},
    {"testlamps",   no_argument, &masterArg, TESTLAMPS},
    {"testprobe",   no_argument, &masterArg, TESTPROBE},
    {"setlamps",    no_argument, &masterArg, SETLAMPS},
    {"maxexposure", no_argument, &masterArg, MAX_EXPOSURE},
    {"sandbox",     no_argument, &masterArg, SANDBOX},
    
    // sampling config
    {"sample_uni_old", no_argument, &masterArg, SAMPLE_UNI_OLD},
    {"sample_uni", no_argument, &masterArg, SAMPLE_UNI},
    {"sample_file", no_argument, &masterArg, SAMPLE_FILE},
    {"sample_all", no_argument, &masterArg, SAMPLE_ALL},
    
    // components config
    {"x11source",   no_argument, &masterArg, X11SOURCE},
    {"imagesource", no_argument, &masterArg, IMAGESOURCE},
    {"sticks",      no_argument, &masterArg, STICKS},
    {"vlamps",      no_argument, &masterArg, VIRTUAL_LAMPS},
    {"probe",       no_argument, &masterArg, LIGHTPROBE},
    {0, 0, 0, 0}
};



/**
 *  Parse command line arguments and set the program configuration accordingly.
 * 
 *  @param argc Forwarded argc from main()
 *  @param argv Forwarded argv from main()
 */
void parseArgs(int argc, char *argv[])
{
    int option_index = 0, c=0;
    while ( ( c = getopt_long (argc, argv, "x:d:i:n:s:c:a:h:r:f:l:b:p:u:o:t:m:w:qeyg3", longOptions, &option_index)) != -1)
    {
        string tmp;
        stringstream arg;
        
        // get argument if not empty
        if (c) arg << (optarg);
        
        // use shortopt depending on longopt (masterarg is set up by getopt_long [see longOptions struct] )
        switch (masterArg) {
            
            case TESTLAMPS: 
            case TESTPROBE: 
            case MAX_EXPOSURE:
            case SANDBOX: progmode = masterArg; break;
            
            case SETLAMPS:  progmode = SETLAMPS; 
                switch (c) {
                    case 's':
                        int lampID; double val;
                        arg >> lampID; arg >> val;
                        setLampsValues.push_back(pair<int,double>(lampID, val));
                        break;
                    case 'u': arg >> setLampsRGB.r; arg >> setLampsRGB.g; arg >> setLampsRGB.b; break;
                    case 'i': arg >> setLampsFile; break;
                    default: break;
                }
                break;
            
            case CAPTURE:
            case CALIBRATE_LAMPS: progmode = masterArg;
                switch (c) {
                    case 'i': arg >> caliConfig.dataDir; break;
                    case 'r': arg >> caliConfig.captureRate; break;
                    default: break;
                } break;
                
            case TRANSFER_SAMPLER: transferConfig.algorithm = Transfer::params::SAMPLER;
            case TRANSFER: progmode = TRANSFER;
                switch (c) {
                    case 'i': arg >> transferConfig.dataDir; break;
                    case 'r': arg >> transferConfig.rate; break;
                    case 'o': arg >> transferConfig.output; break;
                    case 'g': transferConfig.useAverageScale = true; break;
                    case 's': arg >> transferConfig.targetScale; break;
                    case 'y': transferConfig.dynamicFading=true; break;
                    case 'a': arg >> transferConfig.rampScaleFrom; 
                              arg >> transferConfig.rampScaleTo;
                              arg >> transferConfig.rampScaleSteps;
                              break;
                    case 'e': transferConfig.exec = "gphoto2 --camera \"Canon EOS 5D Mark II\" --capture-image"; break;
                    case 'u':  arg >> tmp; transferConfig.useUniform = true; break;
                    case 'c': arg >> transferConfig.numIterations; break;
                    case 'n': arg >> transferConfig.numRepeats; break;
                    case '3': transferConfig.driveSeparateColors = true; transferConfig.exec = "gphoto2 --camera \"Canon EOS 5D Mark II\" --capture-image"; break;
                    default : break;
                } break;
            
            case SAMPLE_UNI_OLD:    samplingConfig.samplingMode = Lightprobe::samplingParams::UNIFORM_OLD;
				switch (c) {
					case 'h':  arg >> samplingConfig.numSamplesH;   break;
					case 'a':  arg >> samplingConfig.numSamplesA; break;
                    case 's':  samplingConfig.kernelMode = Lightprobe::samplingParams::GAUSS; arg >> samplingConfig.coneSigma; break;
                    case 'c':  arg >> samplingConfig.coneSize; break;
                    case 'm':  arg >> samplingConfig.minConeSize; break;
					default: break;
				} break;
                
            
            case SAMPLE_UNI:    samplingConfig.samplingMode = Lightprobe::samplingParams::UNIFORM;
				switch (c) {
                    case 'n':  arg >> samplingConfig.numSamples; break;
                    case 's':  samplingConfig.kernelMode = Lightprobe::samplingParams::GAUSS; arg >> samplingConfig.coneSigma; break;
                    case 'c':  arg >> samplingConfig.coneSize; break;
                    case 'm':  arg >> samplingConfig.minConeSize; break;
					default: break;
                } break;

            case SAMPLE_FILE:    samplingConfig.samplingMode = Lightprobe::samplingParams::FROM_FILE;
				switch (c) {
                    case 'n':  arg >> samplingConfig.numSamples; break;
                    case 'i':  arg >> samplingConfig.filename; break;
                    case 's':  samplingConfig.kernelMode = Lightprobe::samplingParams::GAUSS; arg >> samplingConfig.coneSigma; break;
                    case 'c':  arg >> samplingConfig.coneSize; break;
                    case 'm':  arg >> samplingConfig.minConeSize; break;
					default: break;
                } break;
                

            case SAMPLE_ALL:    
                samplingConfig.samplingMode = Lightprobe::samplingParams::ALLPIXELS;
                samplingConfig.kernelMode = Lightprobe::samplingParams::NONE;
                break;
            
            case X11SOURCE:
                sourceType = X11SOURCE;
                switch (c) {
                    case 'x': arg >> x11sourceConfig.x11Display; break;
                    case 'a': arg >> x11sourceConfig.captureArea.x;
                              arg >> x11sourceConfig.captureArea.y;
                              arg >> x11sourceConfig.captureArea.w;
                              arg >> x11sourceConfig.captureArea.h;
                              break;
                    case 'r': arg >> x11sourceConfig.updateRate;   break;
                    default: break;
                } break;
                
            case IMAGESOURCE:
                sourceType = IMAGESOURCE;
                switch (c) {
                    case 'i': arg >> imagesourceConfig.imagePath; break;
                    default: break;
                } break;
                
            
            case VIRTUAL_LAMPS:
                switch (c) {
                    case 'n': arg >> numVirtualLamps;    break;
                    default : break;
                } break;
            
            case STICKS:
                useSticks = true;
                switch (c) {
                    case 'f': arg >> sticksConfig.fadeSpeed;    break;
                    case 'd': arg >> sticksConfig.serialDevice; break;
                    case 's': arg >> sticksConfig.segmentSize; stickArgs=true; break;
                    case 'l': arg >> sticksConfig.stickSize;  stickArgs=true; break;
                    case 'n': arg >> sticksConfig.numSticks; stickArgs=true;  break;
                    default: break;
                } break;
                
            case LIGHTPROBE: 
                probeArgs = true;
                switch (c) {
                    case 'l':  probeConfig.load(arg.str()); break;
                    case 'c':  arg >> probeConfig.sphereCircle.x;
                               arg >> probeConfig.sphereCircle.y;
                               arg >> probeConfig.sphereCircle.r;
                               break;
                    case 'd':  arg >> probeConfig.camDistance; break;
                    case 'r':  arg >> probeConfig.sphereRadius; break;
                    case 'h':  arg >> probeConfig.horizonAngle; probeConfig.horizonAngle *= M_PI/180.0; break;
                    
                    case 'a':  arg >> probeConfig.rotation(0); probeConfig.rotation(0) *= M_PI/180.0;
                               arg >> probeConfig.rotation(1); probeConfig.rotation(1) *= M_PI/180.0;
                               arg >> probeConfig.rotation(2); probeConfig.rotation(2) *= M_PI/180.0;
                               break;
                    case 't':  arg >> probeConfig.type;
                               break;                               
                    case 'o':  arg >> probeConfig.responseCurve; break;
                    case 'm':  arg >> probeConfig.maskFile; break;
					case 'w':  arg >> probeConfig.whitepoint.r;
							   arg >> probeConfig.whitepoint.g;
							   arg >> probeConfig.whitepoint.b;
							   break;
                    default: break;
                    
                } break;
                
            default: break;
          }
    }
}


/**
 * Add segfault handler and print a backtrace using backtrace() (a feature available in gcc)
 * 
 * @param sig signal number
 */
void segfaultHandler(int sig) {
  void *array[10];
  size_t size;
  size = backtrace(array, 10);

  // print to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}


/**
 * Program entry point. 
 * 
 * Main() parses the command line arguments and sets up all our classes using the specified configurations.
 * It then runs the specified program mode.
 * 
 * @param argc Number of arguments.
 * @param argv Arguments
 * 
 */
int main(int argc, char *argv[])
{
    // install segfault handler
    signal(SIGSEGV, segfaultHandler); 
    
    // force flush after every <<
    std::cout.setf( std::ios_base::unitbuf );
    
    // process commandline
    parseArgs(argc, argv);
    
    // all the important objects
    Lamps* lamps;                       // abstract lamp class.. 
    LampPool* pool;                     // .. that either holds a pool of lamps ..
    Sticks* sticks;                     // .. or an instance of our lighting system model ..
    VirtualLamps* vlamps;               // .. or virtual lamps (lamps w/o hardware. not really used, but it is available).
    Source* source;                     // image source
    Lightprobe* probe;                  // light probe model (uses source as input)
    
    // tell xserver we have threads
    XInitThreads();
    
    //
	// init lamp pool
    //
    pool = new LampPool();
    pool->setUpdateRate(30);
    if (useSticks) {
        if (not stickArgs) {
            std::stringstream sticksConfigFile;
            sticksConfigFile << transferConfig.dataDir << "/sticks.cfg";
            sticksConfig.load(sticksConfigFile.str());
        }
        sticks = new Sticks (sticksConfig);
        pool->addMember(sticks);
        
        
    }
    if (numVirtualLamps > 0) {
        if (transferConfig.useUniform) { Log().err() << "uniform lighting only works with rgb sticks!"; }
        vlamps = new VirtualLamps (numVirtualLamps);
        pool->addMember(vlamps);
    }
    lamps = dynamic_cast<Lamps*>(pool);
    
    // uniform lighting is special because our sticks class has at least three lamps per stick and does not support uniform lighting
    if (transferConfig.useUniform) {
        Log().msg() << "using uniform lighting" << endl;
    } else {
        Log().msg() << "using " << lamps->getNumLamps() << " light sources";
    }
    
    //
    // setlamps progmode handled here because probe or source classes do not have to be specified
    //
    if (progmode == SETLAMPS) {

        lamps->setAll (0);
        lamps->setFadeSpeed(0);
       
        // set lamp values from file
        if (not setLampsFile.empty()) {
            ifstream in;
            in.open(setLampsFile.c_str(), ios::in);
            double val;
            for (int i=0; i<lamps->getNumLamps(); i++) {
                val=-1;
                in >> val;
                assert (val <= 1.0);
                assert (val >= 0.0);
                lamps->setValue(i, val);
            }
            in.close();
        }
        
        // set single lamps from arguments
        for (int i=0; i<setLampsValues.size(); i++) {
            lamps->setValue(setLampsValues[i].first, setLampsValues[i].second);
        }   
        
        // NOTE setLamps RGB only works for lamps of type sticks
        if (not (setLampsRGB.r == 0 && setLampsRGB.g == 0 && setLampsRGB.b == 0)) { 
            sticks->setAllRGB(setLampsRGB);
        }
        
        sticks->send();
        sleep(1);
        delete lamps;
        return 0;
    }
    
        
	
    //
    // init image source
    //
    if (sourceType == IMAGESOURCE) {
        source = new ImageSource(imagesourceConfig);
    } else {
        source = new X11Source(x11sourceConfig);
    }
    
    
    //
    // testlamps  and  max_exposure do not need a lightprobe
    // 
    if (progmode == TESTLAMPS) {
        TestLamps test(lamps, source);
        test.run();
        
    } else if (progmode == MAX_EXPOSURE) {
		MaxExposure max (source);
		max.run();
    
    // all other modes use the lightprobe
    } else {
        
        //
        // init main lightprobe
        //     
        
        // load probe spec from data dir if not specified via commandline
        if (not probeArgs && (not imagesourceConfig.imagePath.empty())) {
            std::stringstream probeConfigFile;
            probeConfigFile << imagesourceConfig.imagePath << "/lightprobe.cfg";
            probeConfig.load(probeConfigFile.str());
        }
        
        // if needed, run probe exposure maximization
        if (probeConfig.exposure == 0) {
            MaxExposure maxExp (source);
            maxExp.run();
            probeConfig.exposure = maxExp.getExposure();
		}
            
		// set camera exposure
		stringstream cmd;
		cmd << "uvcdynctrl -d /dev/video0 -s \"Exposure (Absolute)\" " << (int)probeConfig.exposure;
		system(cmd.str().c_str());
        
        // run gui sphere selection if not already set on commandline
        if (not probeConfig.sphereCircle.isValid()) {
			
            Gui gui (source->getWidth(), source->getHeight());
            Log().msg() << " >> Please mark the sphere in the image: select three points right on the edge to mark a circle.";
            probeConfig.sphereCircle = gui.runSphereSelection(source);
        }
            
        // if needed, ask user to select whitepoint for the input source
        if (not (probeConfig.whitepoint.r > 0 && probeConfig.whitepoint.g > 0 && probeConfig.whitepoint.b > 0) ) {
            Gui gui (source->getWidth(), source->getHeight());
            Log().msg() << " >> Please select a point on a region that should be white.";
            point pixel = gui.runPixelSelection (source);
            probeConfig.whitepoint = sampleGauss7 (source->getImage(), pixel.x, pixel.y);
            Log().msg() << " -> whitepoint is: r=" << probeConfig.whitepoint.r << " g=" << probeConfig.whitepoint.g << " b=" << probeConfig.whitepoint.b;
        }

        probe = new Lightprobe (source, probeConfig, samplingConfig); 
            
        // all lamps 50% on startup
        lamps->setFadeSpeed(0);                         // set 
        lamps->setAll(0.1);
        lamps->send();
        lamps->setFadeSpeed(sticksConfig.fadeSpeed);    // reset
        
        // 
        // start tasks
        //
        
        switch (progmode) {
            case TRANSFER:
                {
                    Transfer transfer (probe, lamps, transferConfig);
                    transfer.run(); 
                    break;  
                } 
            case CAPTURE:
                {            
                    Calibrate calibrate (probe, lamps, caliConfig);
                    calibrate.runCaptureImpacts();
                    break;
                }
            case CALIBRATE_LAMPS:
                {            
                    Calibrate calibrate (probe, lamps, caliConfig);
                    calibrate.runCalibrateLamps();
                    break;
                }
            case TESTPROBE:
                {
                    TestProbe test (probe);
                    test.run(); 
                    break;  
                } 
                 
            case SANDBOX: 
                { 
                    Sandbox box (source, probe, lamps);
                    box.run();
                    break;
                }
            default : break;
        }	
        
	}
    
    
	// all done, exit
    return 0;
}

