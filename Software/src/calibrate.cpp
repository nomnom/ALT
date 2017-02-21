/** 
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * The Calibration Loop for use with the webcam probe. Can also measure the response curve of our lighting system.
 *
 */

#include "calibrate.h"


Calibrate::Calibrate (Lightprobe* p, Lamps* l, params c): probe(p), lamps(l), config(c) {};
Calibrate::Calibrate (Lightprobe* p, Lamps* l, string path, double rate) : probe(p), lamps(l)
{
    config.dataDir = path;
    config.captureRate = rate;    
}
Calibrate::~Calibrate() {};
Calibrate::params Calibrate::getConfig() { return config; }


/**
 * Run the calibration loop using the webcam.
 */
void Calibrate::runCaptureImpacts()
{
    double etime = 1.0/config.captureRate;
    
    const int numRepeats = 2;
    
    Sticks* sticks = dynamic_cast<Sticks*> (lamps);
    int stickLength = sticks->getStickLength(0);
    int width = probe->getSource()->getWidth();
    int height = probe->getSource()->getHeight();
    
    Gui gui(30+width*2, 30+height+100);
    
    // disable fading
    lamps->setFadeSpeed(0);
    
    // save probe config 
    probe->getConfig().save(config.dataDir + "/lightprobe.cfg");
    
	Log().msg() << "impact capture starts in 2 seconds" << endl;
	sleep(2);
    Log().msg() << "impact capture started" << endl;
    
    //
    // capture single darkframe
    //
	
	Log().msg() << "capturing darkframe";
	lamps->setAll(0);
	lamps->send();
	
	// wait for camera
	usleep((long)(2*etime*1000000));
	if (not probe->hasNewData()) { 
		Log().err() << "lightprobe has no new data! You are capturing too fast!";
	}
	probe->acquire();
	imglib::Image<float> darkframe (probe->getSource()->getImage());
    
	// dump darkframe
	stringstream ss;
	ss << config.dataDir << "/img_0.ppm";
	Log().msg() << "dumping darkframe to " << ss.str() << endl;
	darkframe.save(ss.str());
    
    imglib::Image<float> img;
	
	
    //
    // capture lamp impacts
    //
     
    lamps->setAll(0);
    
    for (int i=0; i<lamps->getNumLamps(); i++) {

		// switch on next lamp and send values to device
		Log().msg() << "driving lamp " << (i+1);
		lamps->setValue(i,1);
		lamps->send(); 
		
		// accumulate multiple frames
		for (int r=0; r<numRepeats; ++r) {
			
			
			// wait for camera
			usleep((long)(etime*1000000));
			
			Log().msg() << "capturing frame " << (r+1) << " of " << numRepeats;
			probe->acquire();
			
			// subtract darkframe and accumulate result
			if (r == 0) {
				img = imglib::Image<float>(probe->getImage());
				img = imgSub(img, darkframe);
			} else {
				img = imgAdd(img, probe->getImage());
				img = imgSub(img, darkframe);
			}


            // update gui
            glClear(GL_COLOR_BUFFER_BIT);
            gui.drawCircularImage(probe->getImage(), probe->getConfig().sphereCircle);
            gui.drawImage(img, 20+width, 10);
            gui.drawStickRGBGrid (lamps, 10, 20+height, (2*width+10) / stickLength-2, 100/8-2, 2, true);
            glfwSwapBuffers();
        
		}

		lamps->setValue(i,0);
        
        // dump images
		stringstream ss;
        ss << config.dataDir << "/img_" << (i+1) << ".ppm";
        Log().msg() << "dumping image to " << ss.str() << endl;
        img.save(ss.str());
        

        glfwPollEvents();
        if (glfwGetKey(GLFW_KEY_ESC) == 1) {
            Log().msg() << "user aborted" << endl;
            break;
        }
        

    }

	// all lamps on signals end of calibration
	lamps->setAll(0.5);
	lamps->send();
	
    Log().msg() << "finished capturing impacts" << endl;    
}



/**
 * Run the Lamp-Calibration and calculate LED response using an image source and prerecorded images.
 */
void Calibrate::runCalibrateLamps() {
    
    
    double etime = 1.0/config.captureRate;
    
    const int numRepeats = 2;
    
    Sticks* sticks = dynamic_cast<Sticks*> (lamps);
    int stickLength = sticks->getStickLength(0);
    int width = probe->getSource()->getWidth();
    int height = probe->getSource()->getHeight();
    
    Gui gui(30+width*2, 30+height+100);
    
    // disable fading
    lamps->setFadeSpeed(0);
    
    //
    // capture single darkframe
    //
	
	Log().msg() << "capturing darkframe";
	lamps->setAll(0);
	lamps->send();
	
	probe->acquire();
	imglib::Image<float> darkframe (probe->getSource()->getImage());
    
    
    imglib::Image<float> img;
    
    //Log().msg() << ">> please mark the sampling-area";
    //circle area = gui.runSphereSelection(probe->getSource());
    circle area(100,100,100);
    
    
    const int numSteps = 16;
    double points[3][numSteps][2];
    double maxValue[] = {0,0,0};
    
    // for each color channel..
    for (int c=0; c<3; c++) {
            
        for (int v=0; v<numSteps; v++) {
            double value = (double)(v+1)/(double)(numSteps);
             
            int cc;
            // switch on all lamps of the specified colorchannel
            switch (c) {
                case 0: sticks->setAllRGB( rgb(0,value,0) ); cc= 1; break;
                case 1: sticks->setAllRGB( rgb(value,0,0) ); cc=0;break;
                case 2: sticks->setAllRGB( rgb(0,0,value) ); cc=2; break;
                default: break;
            }
            lamps->send();
            
			// wait for sticks and camera
			usleep((long)(etime*1000000));
            
            probe->acquire();
            img = imglib::Image<float>(probe->getImage());
            //img = imgSub (img, darkframe);  // already accounted for in input images
                            

            // sum up values in specified area and average
            double sum=0; int numSamples=0;
            
            for (int y=area.y-area.r; y<area.y+area.r; ++y)
                for (int x=area.x-area.r; x<area.x+area.r; ++x)
                    if ((area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) < area.r*area.r) {
                        sum += img(x,y,cc)-darkframe(x,y,cc);
                        numSamples ++;
                    }
                        
            sum /= (double)numSamples;  
                        
            
            if (sum > maxValue[c]) maxValue[c] = sum;
            
           
            
            Log().msg() << "-> got " << sum;
            
            // points on response curve
            points[c][v][0] = value;
            points[c][v][1] = sum;
            
            // update gui
            glClear(GL_COLOR_BUFFER_BIT);
            gui.drawCircularImage(img, area);
            gui.drawStickRGBGrid (lamps, 10, 20+height, (2*width+10) / stickLength-2, 100/8-2, 2, true);
            glfwSwapBuffers();
            
                
            glfwPollEvents();
            if (glfwGetKey(GLFW_KEY_ESC) == 1) {
                Log().msg() << "user aborted" << endl;
                break;
            }
        }        
    }
    
    
    
    // print/dump curve data
    for (int c=0; c<3; c++) {		
        cout << endl <<  "  " << c << endl;
        ofstream fout;
        stringstream fname;
        string cname;
        switch (c) { case 0: cname="g"; break; case 1: cname="r"; break; case 2: cname="b"; break; }
        fname << "tmp/response_lamp_raw-" << cname << ".dat";
        fout.open(fname.str().c_str());
        for (int v=0; v<numSteps; ++v) {
            cout <<  points[c][v][0] << " " <<  points[c][v][1] << endl;
            fout <<  points[c][v][0] << " " <<  points[c][v][1] << endl;
        }
    }
    
    Log().msg() << "lamp calibration finished";
}

