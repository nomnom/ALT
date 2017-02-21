/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Implements the Ambient Light Transfer loop.
 *
 */

#include "transfer.h"

Transfer::Transfer (Lightprobe* p, Lamps* l, params c) : 
        probe(p), lamps(l), config(c)
{
    if (config.useUniform) { numLamps = 3; } 
    else { numLamps=lamps->getNumLamps(); }
    numDirs = probe->samplingDirs.size();
    loadImpactData (); 
    numSamples = numDirs*3;
};

Transfer::~Transfer(){
    delete caliProbe;
};


/**
 * Starts the Ambient Light Transfer
 */
void Transfer::run()
{   
    
    
    // old code for converting light probe images with debevec parametrization to our sphere-mapped image format.
    /*
    // dump debevec w/ our parametrisation
    imglib::Image<float> probeImage (impactImages[0].getWidth(),impactImages[0].getHeight(), 3);
    probe->acquire();
    imglib::Image<float> debevec = probe->getImage();
    for (int p=0; p < caliProbe->allPixels.size(); p++) {
        Vector2i pos = caliProbe->allPixels[p];
        Vector3d dir = caliProbe->allDirs[p];
        dir.normalize();
        // debevec paramtrisation
        double r = (1.0/M_PI)*acos(dir[2])/sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
        Vector2i deb_pos;
        deb_pos[0] = ((dir[0]*r + 1.0)/2.0*500.0);
        deb_pos[1] = ((-dir[1]*r + 1.0)/2.0*500.0);
        
        for (int c=0; c<3; c++) {
            probeImage(399-pos[0],pos[1],c) = debevec(deb_pos[0],deb_pos[1],c);
        }
        
    }
    probeImage.save("debevec.ppm");
    //exit(1);
    */
    
    drawTarget = true;
    drawSamplingCones = 4;
    drawPseudoResult = false;
    drawPseudoResultCones = true;
    drawDifference = false;
    doAutoAdjust = false;
    drawScalingFactor = 1.0;
    keyPressFlag = 0;
    
    scaleImpact = false;
    lowPrecision = false;
    resetWeights = false;
    
    double lastTime, delTime, iteration = 0;
    timespec tstart, tend;                    // for high precision stopwatch
    double avgSamplingTime=0, avgAlgoTime=0;  // average processing times
    
    double fps=config.rate;
    double targetScale = config.targetScale;
    double brightness = 1.0;
    
    double probeRotation = 0;
    
    double rampScaleStepSize = 0;
    if (config.rampScaleFrom != config.rampScaleTo) {
        rampScaleStepSize = (config.rampScaleTo - config.rampScaleFrom) / (double)(config.rampScaleSteps-1);
        targetScale = config.rampScaleFrom;
    }
    
                    
    LampPool* pool = dynamic_cast<LampPool*>(lamps);        // oh gawd, this is ugly
    Sticks* sticks = dynamic_cast<Sticks*>(pool->getMember(0));

    // allocate space for optimization data
    weights = new double[numLamps];
    targetData = new double[numDirs*3];
    impactData = new double[numDirs*numLamps*3];   // memory layout is like arr[colorIndex][dirIndex][lampIndex]
    
    for (int l = 0; l < numLamps; ++l) { weights[l] = 0.50; }

    // start gui
    width = probe->getSource()->getWidth(); 
    height = probe->getSource()->getHeight();
    width_cali = impactImages[0].getWidth();
    height_cali = impactImages[0].getHeight();
    gui = new Gui (40+width+2*width_cali, 100+max(height,height_cali)+30);
    
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
    if (config.algorithm == params::OPT) {
#ifdef USE_CERES
        prepareDataCeres();
#else
        prepareDataCVXOPT();
#endif
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
    Log().msg() << " preparation took " << ((double)tend.tv_nsec/1e9 + (double)tend.tv_sec) - ((double)tstart.tv_nsec/1e9 + (double)tstart.tv_sec) << " seconds";

    samplingDirectionsNearestPixel.clear();
    for (int d=0; d<numDirs; d++) {
        if (probe->getSamplingConfig().samplingMode == Lightprobe::samplingParams::ALLPIXELS) {
            samplingDirectionsNearestPixel.push_back(d);
        } else {
            samplingDirectionsNearestPixel.push_back(findNearestNeighbor(probe->samplingDirs[d], probe->allDirs));
        }
    }

    
    ofstream pout;  // plot ostream
    ofstream lout;  // lamps ostream
    if (not config.output.empty()) {
        std::stringstream plotfile;
        plotfile << config.output << ".plot";
        pout.open(plotfile.str().c_str(), ios::out);
        pout << "# 1:iter  2:err  3:#dirs  4:scale  5:ascale  6:err/s^2  7: err/s^2/d  8: avgweights  9:rt_sum  10:rt_samp_avg  11:rt_opt_avg " << endl;
        exp_plot_kernel(probe->samplingCones);
        
        std::stringstream lampfile;
        lampfile << config.output << ".lamps";
        lout.open(lampfile.str().c_str(), ios::out);
    }
    
    // start fading thread
    if (sticks->getConfig().fadeSpeed  > 0) lamps->start();
    
    Log().msg() << "Ambient Light Transfer started" ;


    // main ALT loop
    while (glfwGetKey(GLFW_KEY_ESC) != 1) {
        
        lastTime = glfwGetTime(); 
        iteration++;
        
        if (resetWeights)
            for (int l = 0; l < numLamps; ++l) { weights[l] = 0.50; }

        // get target lighting conditions
        probe->acquire();    
        targetImage = probe->getImage();
        
        double appliedScale =  targetScale * averageBrightness;
        
        // repeat sampling for averaged runtime results
        avgSamplingTime=0;
        for (int r=0; r<config.numRepeats; r++) {
                
    
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
            targetImpact = probe->getImpact(targetImage);
            
            // zero out unused directions
            for (int d=0; d<numDirs; d++) {
                if (caliProbe->usedDirections[d] && probe->usedDirections[d]) {
                    targetImpact[d].r = ( targetImpact[d].r  * appliedScale );
                    targetImpact[d].g = ( targetImpact[d].g  * appliedScale ) ;
                    targetImpact[d].b = ( targetImpact[d].b  * appliedScale );
                    
                } else {
                    targetImpact[d] = rgb(0,0,0);
                }
            }
            
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
            Log().msg() << " sampling took " << ((double)tend.tv_nsec/1e9 + (double)tend.tv_sec) - ((double)tstart.tv_nsec/1e9 + (double)tstart.tv_sec) << " seconds";

            avgSamplingTime += ((double)tend.tv_nsec/1e9 + (double)tend.tv_sec) - ((double)tstart.tv_nsec/1e9 + (double)tstart.tv_sec);
        }
        Log().msg() << "sampling average " << avgSamplingTime /  (double)(config.numRepeats) << " seconds";

        // repeat optimizer for averaged runtime results
        avgAlgoTime=0;
        for (int r=0; r<config.numRepeats; r++) {
                    
            
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
            if (config.algorithm == params::OPT) {
                // run optimizer

    #ifdef USE_CERES        
                runCeres();
    #else
                runCVXOPT();
    #endif
            } else if (config.algorithm == params::SAMPLER) {
                
                // run sampling algo
                for (int l=0; l<numLamps; l++) {
                    rgb result(0,0,0);
                    for (int d=0; d<numDirs; d++) {
                        if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
                        result.r += targetImpact[d].r * lightImpacts[l][d].r;
                        result.g += targetImpact[d].g * lightImpacts[l][d].g;
                        result.b += targetImpact[d].b * lightImpacts[l][d].b;
                        
                    }
                    weights[l] = result.r + result.g + result.b;
                }
            }
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
            Log().msg() << " preparation took " << ((double)tend.tv_nsec/1e9 + (double)tend.tv_sec) - ((double)tstart.tv_nsec/1e9 + (double)tstart.tv_sec) << " seconds";
            
            avgAlgoTime += ((double)tend.tv_nsec/1e9 + (double)tend.tv_sec) - ((double)tstart.tv_nsec/1e9 + (double)tstart.tv_sec);
        }
        
        
        Log().msg() << "optimization average " << (avgAlgoTime / (double) (config.numRepeats) ) << " seconds";
        
        Log().log(2) << "result:";
        stringstream ss;
        ss << "weights:";
        for (int i=0; i<numLamps; i++) { 
			//if ( (i % (sticks->getStickLength(0)*3) == 0)) ss << endl;
			ss << " " << weights[i];
		}
        Log().log(2) << ss.str();
        
        ss.str(""); ss.clear();
        ss << "sampling target:";
        for (int d=0; d<numDirs; d++) { 
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
                for (int c=0; c<3; c++)
                    ss << " " << targetImpact[d].getVec()[c];
		}
        Log().log(2) << ss.str();
        
        // clip weights and detect maxed out values
        bool weightClipping = false;
        bool maxedOut = false;
        for (int i=0; i<numLamps; i++) { 
            weights[i] *= brightness;
            if (weights[i] > 1.0) { weightClipping = true; }
            if (weights[i] >= 0.999999) { maxedOut = true; }
            weights[i] = clamp(weights[i]);
        }
        
        // set lamp values; handle uniform lighting separately
		if (config.useUniform) {
			sticks->setAllRGB(rgb(weights[1], weights[0], weights[2]));  // weights in G/R/B order
		} else {
			for (int l=0; l<numLamps; l++) {
				lamps->setValue(l, weights[l]);
			}
		}
    
        // drive lamps (all colors at once, or one channel after the other.
        // executes a system command between each iteration.
        for (int c=0; c<3; c++) {
            
            
            // normal mode: continue with all channels on
            if (not config.driveSeparateColors) {
                if (sticks->getConfig().fadeSpeed == 0) lamps->send();
                break;
                
            // separate color driving: zero channel #c, execute gphoto command, repeat    
            } else {

                switch (c) {
                    case 0:  sticks->setAllChannel(0.0, 1);  sticks->setAllChannel(0.0, 2); break; // green
                    case 1:  sticks->setAllChannel(0.0, 0);  sticks->setAllChannel(0.0, 2); break; //red
                    case 2:  sticks->setAllChannel(0.0, 0);  sticks->setAllChannel(0.0, 1); break; //blue
                }
                
                if (sticks->getConfig().fadeSpeed  == 0) lamps->send();
                
                cout << "executing capture command " <<  config.exec << endl;
                sleep(1);
                system(config.exec.c_str());
                
            }
            
            
            if (config.useUniform) {
                sticks->setAllRGB(rgb(weights[1], weights[0], weights[2]));  // weights in G/R/B order
            } else {
                for (int l=0; l<numLamps; l++) {
                    lamps->setValue(l, weights[l]);
                }
            }
        }
            
        ss.str(""); ss.clear();
        ss << "sampling result:";
        
        // calculate error
        double minErr=0;
        int numDirsSampled=0;
        for (int d=0; d<numDirs; ++d) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            
            numDirsSampled++;
            double res;
        
            for (int c=0; c<3; c++) {
                double sum = 0;
                for (int l=0; l<numLamps; ++l) {
                    sum += lightImpacts[l][d].getVec()[c]*weights[l];;
                }
                ss << " " << sum;
                sum -= targetImpact[d].getVec()[c];
                minErr += sum*sum;
            }
        }
        Log().log(2) << ss.str();
            

        // framewait if needed
        delTime = glfwGetTime() - lastTime;
        if (delTime < 1.0/fps) {
            usleep((long)(( 1.0/fps - delTime)*1000000));
        }
        
        // set window title (only text output besides COUT)
        std::stringstream title;
        title.precision(4); title.setf( std::ios::fixed, std:: ios::floatfield );
        title << "ALT  ( err=" << minErr << ",  err/s^2=" << (minErr/appliedScale/appliedScale)<< ", err/s^2/#d=" << (minErr/appliedScale/appliedScale/(double)numDirsSampled) << ", ";
        title.precision(2); title.setf( std::ios::fixed, std:: ios::floatfield );
        title << (1/delTime) << " maxFPS ";
        title << "targetScale = " << targetScale << ", brightness=" << brightness << (scaleImpact?", SCALE":"") <<  (lowPrecision?", LOWP":"")  <<  (resetWeights?", RES":"") << " )";        
        if (maxedOut) title << "  MAX!";
        if (weightClipping) title << "  CLIP!";
        glfwSetWindowTitle(title.str().c_str());
    
        // draw GUI
        repaint();
        
        // process keyboard events
        glfwPollEvents();
        drawTarget = toggleByKey(drawTarget, GLFW_KEY_F1); 
        drawPseudoResult = toggleByKey(drawPseudoResult, GLFW_KEY_F3); 
        drawPseudoResultCones = toggleByKey(drawPseudoResultCones, GLFW_KEY_F4); 
        drawDifference = toggleByKey(drawDifference, GLFW_KEY_F5); 
        
        scaleImpact = toggleByKey(scaleImpact, GLFW_KEY_F9); 
        lowPrecision = toggleByKey(lowPrecision, GLFW_KEY_F10); 
        resetWeights = toggleByKey(resetWeights, GLFW_KEY_F11); 
        
        if (glfwGetKey(GLFW_KEY_F2) == 1 && keyPressFlag == 0) { 
            keyPressFlag=GLFW_KEY_F2; 
            drawSamplingCones += 1; drawSamplingCones = drawSamplingCones % 7;
        } else if (glfwGetKey(GLFW_KEY_F2) == 0 && keyPressFlag == GLFW_KEY_F2)  { 
            keyPressFlag=0;
        }  
            
        if (glfwGetKey(GLFW_KEY_KP_7) == 1) { targetScale *= 1.1; }
        if (glfwGetKey(GLFW_KEY_KP_4) == 1) { targetScale = 1.0; }
        if (glfwGetKey(GLFW_KEY_KP_1) == 1) { targetScale /= 1.1; }
        
        if (glfwGetKey(GLFW_KEY_KP_8) == 1) { brightness *= 1.1; }
        if (glfwGetKey(GLFW_KEY_KP_5) == 1) { brightness = 1.0; }
        if (glfwGetKey(GLFW_KEY_KP_2) == 1) { brightness /= 1.1; }
        
        if (glfwGetKey(GLFW_KEY_LEFT) == 1) { probeRotation += M_PI / 4.0; probe->setRotationY(probeRotation); }
        if (glfwGetKey(GLFW_KEY_SCROLL_LOCK) == 1) { probeRotation = 0; probe->setRotationY(probeRotation); }
        if (glfwGetKey(GLFW_KEY_RIGHT) == 1) { probeRotation -= M_PI / 4.0; probe->setRotationY(probeRotation); }
        
        
        // auto adjust brightness
        doAutoAdjust = (glfwGetKey('A') == 1);
        glfwPollEvents();
        
        // if dynamic fading is enabled, set fading speed to the duration of last loop iteration
        delTime = glfwGetTime() - lastTime;
        if (config.dynamicFading) lamps->setFadeSpeed(1.0/(double)delTime);

        // dump result values to log
        stringstream logline;
        if (not config.output.empty()) {
            double avgWeights = 0;
            for (int i=0; i<numLamps; i++) { 
                avgWeights += weights[i];
            }
            avgWeights /= numLamps;
            
            
            logline << iteration << " "  << minErr << " " << numDirsSampled  << " "  << targetScale << " " << averageBrightness << " " <<  minErr / appliedScale / appliedScale << " "  <<  minErr / appliedScale / appliedScale / (double)numDirsSampled << " "  <<  avgWeights << " " << delTime << " " << avgSamplingTime  << " " << avgAlgoTime;
            cout << " iteration << minErr << numDirsSampled  <<  targetScale <<  averageBrightness <<   minErr / appliedScale / appliedScale <<   minErr / appliedScale / appliedScale / numDirsSampled <<  avgWeights <<  delTime <<  avgSamplingTime << avgAlgoTime" << endl;
            cout << logline.str() << endl;
            pout << logline.str() << endl;
            
            // supress expensive image results if ramp scale is used
            if (config.rampScaleFrom == config.rampScaleTo) {
                createResults(iteration);
            }
            
			for (int l=0; l<numLamps; l++) {
				lout << weights[l] << " ";
			}
            lout << endl;
        }
        
        // execute command
        if (not config.exec.empty() && not config.driveSeparateColors ) {
			cout << "executing system command " <<  config.exec << endl;
            system(config.exec.c_str());
        }
            
        if (iteration == config.numIterations) {
            Log().msg() << "reached max iterations, exiting";
            break;
        }
        
        // if ramp scale : end if max scale is reached
        if (config.rampScaleFrom != config.rampScaleTo) {
            if (iteration == config.rampScaleSteps)  { break; }
            else { targetScale += rampScaleStepSize; }
            
        }
    }
    
    Log().msg() << "done!";
    
    if (sticks->getConfig().fadeSpeed  > 0) lamps->stop();
    sleep(1);     // wait so the threads can terminate
    
    // cleanup
    if (weights != NULL) delete [] weights;
    if (targetData != NULL) delete [] targetData;
    if (impactData != NULL) delete [] impactData;
    if (not config.output.empty()) {
        pout.close();
        lout.close();
    }
    delete gui;

}

/**
 * Returns the inverted value of a boolean iff a key is pressed
 */
bool Transfer::toggleByKey(bool var, int key)
{
    if (glfwGetKey(key) == 1 && keyPressFlag == 0) { 
        keyPressFlag=key; 
        return not var;
    } else if (glfwGetKey(key) == 0 && keyPressFlag == key)  { 
        keyPressFlag=0;
    }  
    return var;
}


/**
 * Dump image results if config.output specifies a directory/prefix
 */
void Transfer::createResults( int iter )
{
    //
    // pseudo result
    //
    imglib::Image<float> resultImage (width_cali, height_cali, 3);
    circle area = caliProbe->getConfig().sphereCircle;
    for (int x=0; x<width_cali; x++)
        for (int y=0; y<height_cali; y++) {
            for (int c=0; c<3; c++) {
                resultImage(x,y,c) = 0;
                if (( (area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) ) < area.r*area.r) {
                    for (int i=0; i<numLamps; i++) {
                        resultImage(x,y,c) = resultImage(x,y,c)  + impactImages[i](x,y,c) * weights[i] * 0.5;  
                    }
                }
            }
        }
    std::stringstream pseudoimg;
    pseudoimg << config.output << "_pseudo-" << iter << ".ppm";
    resultImage.save(pseudoimg.str());
    
    
    //
    // lamp values pixel image
    //
    LampPool* pool = dynamic_cast<LampPool*>(lamps);
    if (pool == NULL) { Log().err() << " encountered wrong lamp type while drawing lamp values"; }
    else {
        // assume first member exists and is sticks
        Sticks* sticks = dynamic_cast<Sticks*>(pool->getMember(0));
        if (sticks == NULL) { Log().err() << "sticks is NULL!!"; }

        imglib::Image<float> lampImage (sticks->getNumSticks(), sticks->getStickLength(0), 3);
        for (int s=0; s<sticks->getNumSticks(); s++) {
            for (int l=0; l<sticks->getStickLength(s); l++) {
                rgb color = sticks->getStickRGBValue(s,l);
                lampImage(s,l,0) = color.r;
                lampImage(s,l,1) = color.g;
                lampImage(s,l,2) = color.b;
            }
        }
        std::stringstream lampimg;
        lampimg << config.output << "_lamps-" << iter << ".ppm";
        lampImage.save(lampimg.str());
    }    
    
    //
    // pseudo result cones
    //
    imglib::Image<float> resultConesImage (width_cali, height_cali, 3);
    for (int d=0; d<numDirs; d++) {
        if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
        
        rgb sum(0,0,0);
        for (int l=0; l<numLamps; l++) {
            sum.r += lightImpacts[l][d].r*weights[l];    // TODO missing brightness
            sum.g += lightImpacts[l][d].g*weights[l];
            sum.b += lightImpacts[l][d].b*weights[l];
        }   
        int x, y;
        for (int p=0; p < caliProbe->samplingCones[d].pixels.size(); ++p) {
            x = caliProbe->samplingCones[d].pixels[p](0);
            y = caliProbe->samplingCones[d].pixels[p](1);
            resultConesImage(x,y,0)=sum.r;
            resultConesImage(x,y,1)=sum.g;
            resultConesImage(x,y,2)=sum.b;
        }            
    } 
    
    std::stringstream resultcones;
    resultcones << config.output << "_pseudo_cones-" << iter << ".ppm";
    resultConesImage.save(resultcones.str());
    
    
    
    //
    // diff cones
    //
    imglib::Image<float> diffConesImage (width_cali, height_cali, 3);
    for (int d=0; d<numDirs; d++) {
        if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
        
        rgb sum(0,0,0);
        for (int l=0; l<numLamps; l++) {
            sum.r += lightImpacts[l][d].r*weights[l];
            sum.g += lightImpacts[l][d].g*weights[l];
            sum.b += lightImpacts[l][d].b*weights[l];
        }   
        sum.r -= targetImpact[d].r;
        sum.g -= targetImpact[d].g;
        sum.b -= targetImpact[d].b;
        
        int x, y;
        for (int p=0; p < caliProbe->samplingCones[d].pixels.size(); ++p) {
            x = caliProbe->samplingCones[d].pixels[p](0);
            y = caliProbe->samplingCones[d].pixels[p](1);
            diffConesImage(x,y,0)=abs(sum.r);
            diffConesImage(x,y,1)=abs(sum.g);
            diffConesImage(x,y,2)=abs(sum.b);
        }            
    } 
    
    std::stringstream diffcones;
    diffcones << config.output << "_diff_cones-" << iter << ".ppm";
    diffConesImage.save(diffcones.str());
    
    
      
    //
    // target cones
    //
    imglib::Image<float> targetConesImage (width_cali, height_cali, 3);
    for (int d=0; d<numDirs; d++) {
        if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
        int x, y;
        for (int p=0; p < caliProbe->samplingCones[d].pixels.size(); ++p) {
            x = caliProbe->samplingCones[d].pixels[p](0);
            y = caliProbe->samplingCones[d].pixels[p](1);
            targetConesImage(x,y,0)=targetImpact[d].r;
            targetConesImage(x,y,1)=targetImpact[d].g;
            targetConesImage(x,y,2)=targetImpact[d].b;
        }            
    } 
    
    std::stringstream targetcones;
    targetcones << config.output << "_target_cones-" << iter << ".ppm";
    targetConesImage.save(targetcones.str());
    
    
    
}

/**
 * Repaints the GUI.
 */
void Transfer::repaint()
{

    // update gui
    glClear(GL_COLOR_BUFFER_BIT);
    
    // target
    if (drawTarget) {
        gui->drawImage(targetImage, 10, 10, false);
        glColor4f(1,1,1,0.5);
        gui->drawCircle(probe->getConfig().sphereCircle,10,10);
    }            
 
    // difference between target and pseudo-result (only works if the same lightprobe config is used for calibration and transfer)
    if (drawDifference) {
        
        // sampling direction based
        // for now unscaled absolute values
        glBegin(GL_POINTS);   
        double  maxDiff=0;
        vector<rgb> diffs;
        for (int d=0; d<numDirs; d++) {
            rgb sum(0,0,0);
            for (int l=0; l<numLamps; l++) {
                sum.r += lightImpacts[l][d].r*weights[l];;
                sum.g += lightImpacts[l][d].g*weights[l];;
                sum.b += lightImpacts[l][d].b*weights[l];;
            }  
            rgb diff;
            diff.r = abs(targetImpact[d].r - sum.r);
            diff.g = abs(targetImpact[d].g - sum.g);
            diff.b = abs(targetImpact[d].b - sum.b);
            maxDiff = max (maxDiff, max(diff.r, max(diff.g, diff.b)));
            diffs.push_back(diff);
        }
        
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            for (int p=0; p < caliProbe->samplingCones[d].pixels.size(); ++p) {
                glColor4f(diffs[d].r/maxDiff, diffs[d].g/maxDiff, diffs[d].b/maxDiff, caliProbe->samplingCones[d].weights[p]);
                glVertex2i (width_cali+width+30+caliProbe->samplingCones[d].pixels[p](0), 10+caliProbe->samplingCones[d].pixels[p](1) );
            }           
            
        } 
        glEnd();
        
    }
    
    
    // display pseudo result
    
    // .. as image
    imglib::Image<float> resultImage (width_cali, height_cali, 3);
    circle area = caliProbe->getConfig().sphereCircle;
    if (drawPseudoResult) {        
        //if (doAutoAdjust) { drawScalingFactor = 1e10; }
        for (int x=0; x<width_cali; x++)
            for (int y=0; y<height_cali; y++)
                if (( (area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) ) < area.r*area.r)
                    for (int c=0; c<3; c++) {
                        for (int i=0; i<numLamps; i++) {
                            resultImage(x,y,c) = resultImage(x,y,c)  + impactImages[i](x,y,c) * weights[i]; 
                        }
                        
                        if (doAutoAdjust) { drawScalingFactor = 1.0 / max( 1.0/drawScalingFactor , (double)resultImage(x,y,c)); }
                    }

        imgMul (resultImage, drawScalingFactor);
        gui->drawImage(resultImage, width+20, 10, false);
        doAutoAdjust=false;
    }
    
    // .. as sampling neighborhood
    if (drawPseudoResultCones) {
        
        if (doAutoAdjust) { drawScalingFactor = 1e10; }
            
        glBegin(GL_POINTS);   
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            
            rgb sum(0,0,0);
            for (int l=0; l<numLamps; l++) {
                sum.r += lightImpacts[l][d].r*weights[l];    // TODO missing brightness
                sum.g += lightImpacts[l][d].g*weights[l];
                sum.b += lightImpacts[l][d].b*weights[l];
            }   
            
            if (doAutoAdjust) {
                drawScalingFactor = 1.0 / max( 1.0/drawScalingFactor , max(sum.r,max(sum.g,sum.b)) );
            }
            
            
            for (int p=0; p < caliProbe->samplingCones[d].pixels.size(); ++p) {
                glColor4f(sum.r*drawScalingFactor, sum.g*drawScalingFactor, sum.b*drawScalingFactor, caliProbe->samplingCones[d].weights[p]);
                glVertex2i (width+20+caliProbe->samplingCones[d].pixels[p](0), 10+caliProbe->samplingCones[d].pixels[p](1) );
            }            
        } 
        glEnd();
    }

    // draw sampling neighborhood
    if (drawSamplingCones == 1) {
        srand(0); // always same colors in each frame
        glBegin(GL_POINTS);   
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            rgb color((float)rand()/(float)RAND_MAX,(float)rand()/(float)RAND_MAX,(float)rand()/(float)RAND_MAX);
            for (int p=0; p < probe->samplingCones[d].pixels.size(); ++p) {
                glColor4f(color.r, color.g, color.b, probe->samplingCones[d].weights[p]);
                glVertex2i (10+probe->samplingCones[d].pixels[p](0), 10+probe->samplingCones[d].pixels[p](1) );
            }
        } 
        glEnd();
        
    } else if (drawSamplingCones == 2) {
        
        rgb color;
        glBegin(GL_POINTS);   
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            color = targetImpact[d];
            for (int p=0; p < probe->samplingCones[d].pixels.size(); ++p) {
                glColor4f(color.r, color.g, color.b, probe->samplingCones[d].weights[p]);
                glVertex2i (10+probe->samplingCones[d].pixels[p](0), 10+probe->samplingCones[d].pixels[p](1) );
            }
        } 
        glEnd();
        
    } else if (drawSamplingCones == 3) {
        
        rgb color;
        glBegin(GL_POINTS);   
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            color = maximumImpacts[d];
            for (int p=0; p < probe->samplingCones[d].pixels.size(); ++p) {
                glColor4f(color.r, color.g, color.b, probe->samplingCones[d].weights[p]);
                glVertex2i (10+probe->samplingCones[d].pixels[p](0), 10+probe->samplingCones[d].pixels[p](1) );
            
            }
        } 
        glEnd();
    
    } else if (drawSamplingCones == 4) {
        rgb color;
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            color = targetImpact[d];
            glColor4f(color.r, color.g, color.b, 1.0);
            int cand = samplingDirectionsNearestPixel[d];
            gui->drawBox(rect(10+probe->allPixels[cand](0), 10+probe->allPixels[cand](1), 4, 4));
        } 
        
    } else if (drawSamplingCones >= 5) {
        
        rgb color;
        glBegin(GL_POINTS);   
        for (int d=0; d<numDirs; d++) {
            if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
            color = rgb2srgb(maximumImpacts[d]);
            for (int p=0; p < probe->samplingCones[d].pixels.size(); ++p) {
                glColor4f(1, 1, 1, probe->samplingCones[d].weights[p]/10);
                glVertex2i (10+probe->samplingCones[d].pixels[p](0), 10+probe->samplingCones[d].pixels[p](1) );
            }
            if (drawSamplingCones == 6) break;
        } 
        glEnd();
        
    }
    
    // draw lamp values (only works for Sticks class)
    LampPool* pool = dynamic_cast<LampPool*>(lamps);
    if (pool == NULL) { Log().err() << " encountered wrong lamp type while drawing lamp values"; }
    else {
        // assume first member exists and is sticks
        Sticks* sticks = dynamic_cast<Sticks*>(pool->getMember(0));
        if (sticks == NULL) { Log().err() << "sticks is nULL!!"; }
        gui->drawStickRGBGrid (sticks, 10, 20+max(height, height_cali), (width+width_cali+10)/ sticks->getStickLength(0)-2, 100/8-2, 2, true);
        
        // assume second member in pool is not sticks
        if (pool->getNumMembers() == 2) {
            Lamps* mono = pool->getMember(1);
            gui->drawMonochromeLamps(mono, 10, 100+20+max(height, height_cali), (width+width_cali+10)/mono->getNumLamps()-2, 20,2,true);
        }
        
    }
    
    glfwSwapBuffers();

    doAutoAdjust = false;
}


/**
 * Prepares the CVXOPT optimizer. Creates all matrices and vectors.
 */
void Transfer::prepareDataCVXOPT()
{
    
    Log().msg() << " initializing CVXOPT and  preparing data";

    // create matrix Q
    Py_Initialize();

  if (import_cvxopt() < 0) {
    Log().err() << "error importing cvxopt";
    return;
  }

  /* import cvxopt.solvers */
  PyObject *solvers = PyImport_ImportModule("cvxopt.solvers");
  if (!solvers) {
    Log().err() << "error importing cvxopt.solvers";
    return;
  }

  /* get reference to solvers.solvelp */
  qpsolver = PyObject_GetAttrString(solvers, "qp");
  if (!qpsolver) {
    Log().err() << "error referencing cvxopt.solvers.lp";
    Py_DECREF(solvers);
    return;
  }
  
  qp_c = (PyObject *)Matrix_New(numLamps, 1, DOUBLE);
  qp_Q = (PyObject *)Matrix_New(numLamps, numLamps, DOUBLE);
  
  qp_A = (PyObject *)Matrix_New(2*numLamps+(2), numLamps, DOUBLE);
  qp_b = (PyObject *)Matrix_New(2*numLamps+(2), 1 , DOUBLE);
  
  for (int l=0; l<numLamps; ++l) {
    
    // b vector
    MAT_BUFD(qp_b)[l] = 1;
    MAT_BUFD(qp_b)[l+numLamps] = 0;
    
    // A Matrix
    for (int ly=0; ly<numLamps; ++ly) {
        if (l == ly) { 
            MAT_BUFD(qp_A)[l*(2*numLamps+2) + ly] = 1;              // upper half
            MAT_BUFD(qp_A)[l*(2*numLamps+2) + ly + numLamps] = -1;  // upper half
        } else { 
            MAT_BUFD(qp_A)[l*(2*numLamps+2) + ly] = 0;              // bottom half
            MAT_BUFD(qp_A)[l*(2*numLamps+2) + ly + numLamps] = 0;   // bottom half
        }
    }
      
    // Q matrix
    for (int ly=0; ly<numLamps; ++ly) {
          
        // fill field (l,ly) of Q
        double val=0;
        for (int c=0; c<3; c++) {
            for (int d=0; d<numDirs; d++) {
                val += lightImpacts[l][d].getVec()[c] * lightImpacts[ly][d].getVec()[c];
            }
        }
        MAT_BUFD(qp_Q)[l*numLamps+ly] = val;
    }
    
    cout << ".";  
    // c vector is filled in runCVXOPT (contains Target)
  }
  cout << endl;
  
  
  
  Log().msg() << " done preparing CVXOPT data";
}

/**
 * Starts the optimization
 */
void Transfer::runCVXOPT()
{
        
    // update target vector (c)
    for (int l=0; l<numLamps; ++l) {
        double val=0;
        for (int d=0; d<numDirs; ++d) {
            val -= lightImpacts[l][d].r*targetImpact[d].r;
            val -= lightImpacts[l][d].g*targetImpact[d].g;
            val -= lightImpacts[l][d].b*targetImpact[d].b; 
        }
        MAT_BUFD(qp_c)[l] = val;
        cout << ".";
    }
    cout << endl;

    qpsolverArgs = PyTuple_New(4);
    PyTuple_SetItem(qpsolverArgs, 0, qp_Q);
    PyTuple_SetItem(qpsolverArgs, 1, qp_c);
    PyTuple_SetItem(qpsolverArgs, 2, qp_A);
    PyTuple_SetItem(qpsolverArgs, 3, qp_b);    
    if (!qp_c || !qp_Q || !qp_c || !qp_A || !qpsolverArgs) {
        Log().err() << "error creating matrices";
    }  
    
    // run cvxopt    
    Log().msg() << "calling qp solver...";
    PyObject *sol = PyObject_CallObject(qpsolver, qpsolverArgs);
    if (!sol) {
        Log().err() << "qp solver failed!";
        PyErr_Print();
    }
  
    PyObject *x = PyDict_GetItemString(sol, "x");

    for (int l=0; l<numLamps; ++l) {
        weights[l] =  MAT_BUFD(x)[l];
        cout << ".";
    }
    cout << endl;
 
    Log().msg() << "qp solver finished";
}



/**
 * Sets up Ceres as optimizer.
 */
void Transfer::prepareDataCeres() {
    
    // create static memory block from vectors
    for (int d=0; d<numDirs; d++)
        for (int l=0; l<numLamps; l++) {
            impactData[0*numDirs*numLamps + d*numLamps + l] = lightImpacts[l][d].r;
        }
    for (int d=0; d<numDirs; d++)
        for (int l=0; l<numLamps; l++) {
            impactData[1*numDirs*numLamps + d*numLamps + l] = lightImpacts[l][d].g;
        }
    for (int d=0; d<numDirs; d++)
        for (int l=0; l<numLamps; l++) {
            impactData[2*numDirs*numLamps + d*numLamps + l] = lightImpacts[l][d].b;
        }
        
#ifdef USE_CERES
    // create static memory block from vectors (rgb triplets)
    for (int d=0; d<numDirs; d++)
        for (int l=0; l<numLamps; l++)
            for (int c=0; c<3; c++) {
                data[d*numLamps*3 + l*3 + c ] = lightImpacts[l][d].getVec()[c];
            }
#endif
    
}

#define MIN_WEIGHT_DISTANCE 0.05

/**
 * Starts the optimization.
 */
void Transfer::runCeres()
{
    
    // reuse weights from last round, but move them if too close at boundaries
    for (int l=0; l<numLamps; l++) {
        if (weights[l] <= MIN_WEIGHT_DISTANCE) weights[l] = MIN_WEIGHT_DISTANCE;
        if (weights[l] >= (1-MIN_WEIGHT_DISTANCE)) weights[l] = (1-MIN_WEIGHT_DISTANCE);
    }
    ceres::Problem problem;
    int numResidues=0;


#ifdef CERES_JACOBI

    // 
    // cost function w/ analytical derivatives (jacobians direclty implemented, not autodiff)
    // 
   for (int d = 0; d < numDirs; ++d) {
       //add residue to solver only iff light direction contains sampled values (target _and_ calibration images)
       if (probe->samplingCones[d].pixels.size() == 0) continue;
       if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
       
       
       
        for (int c=0; c<3; c++) {
            double target;
            if (scaleImpact)  {
                switch (c) {
                    case 0: target = targetImpact[d].r * (maximumImpacts[d].r) ; break;
                    case 1: target = targetImpact[d].g * (maximumImpacts[d].g) ; break;
                    case 2: target = targetImpact[d].b * (maximumImpacts[d].b) ; break;
                }
            } else {
                switch (c) {
                    case 0: target = targetImpact[d].r; break;
                    case 1: target = targetImpact[d].g; break;
                    case 2: target = targetImpact[d].b; break;
                }
            }
            
            
            ceres::CostFunction* cost_function = new CostSimple(target, &(impactData[c*numLamps*numDirs + d*numLamps]) );
            problem.AddResidualBlock(cost_function, NULL, weights);
            numResidues++;
        }
    }
    
    

#else   
    //
    // using auto diff cost functions and one residual function per direction / color
    //

    for (int d = 0; d < numDirs; ++d) {
		for (int c=0; c<3; c++) {
	
			double target;
			if (scaleImpact)  {
				switch (c) {
					case 0: target = targetImpact[d].r * (maximumImpacts[d].r) ; break;
					case 1: target = targetImpact[d].g * (maximumImpacts[d].g) ; break;
					case 2: target = targetImpact[d].b * (maximumImpacts[d].b) ; break;
				}
			} else {
				switch (c) {
					case 0: target = targetImpact[d].r; break;
					case 1: target = targetImpact[d].g; break;
					case 2: target = targetImpact[d].b; break;
				}
			}
            
            ceres::CostFunction* cost_function = new ceres::AutoDiffCostFunction<Residual,1,108>(new Residual( target, numLamps, &(impactData[c*numLamps*numDirs + d*numLamps])));
            problem.AddResidualBlock(cost_function, NULL, weights);
            numResidues++;
        }
    }

#endif
    
    Log().msg() << "running ceres using " << numResidues << " residues";
    
    //
    // ceres options
    // 
    
    ceres::Solver::Options options;
    options.gradient_tolerance = 1e-12;
    options.function_tolerance = lowPrecision ? 1e-6 : 1e-12;
    options.parameter_tolerance = 1e-12;
    options.max_num_iterations = 500;
    //options.max_solver_time_in_seconds = 0.2;
    
#ifndef CERES_JACOBI
    options.num_threads = 4;    // only for jacobian evaluation        
#endif    
    
#ifdef CERES_LINESEARCH    
    
    // line search
    options.minimizer_type = ceres::LINE_SEARCH;
    options.line_search_direction_type = ceres::STEEPEST_DESCENT;          // LBFGS or NONLINEAR_CONJUGATE_GRADIENT or STEEPEST_DESCENT
    options.linear_solver_type = ceres::DENSE_QR; // DENSE_QR or SPARSE_NORMAL_CHOLESKY
    //options.preconditioner_type = ceres::JACOBI; // IDENTITY or JACOBI or CLUSTER_JACOBI
    
    
#else 
    // trust region
    options.minimizer_type = ceres::TRUST_REGION;
    options.trust_region_strategy_type = ceres::DOGLEG;     // DOGLEG or LEVENBERG_MARQUARDT
    options.dogleg_type = ceres::TRADITIONAL_DOGLEG;        // TRADITIONAL_DOGLEG or SUBSPACE_DOGLEG
    //options.initial_trust_region_radius = 0.5;
    //options.max_trust_region_radius = 1e-2;
    //options.use_nonmonotonic_steps = true;
    
#endif
   
    options.minimizer_progress_to_stdout = true;

    Log().msg() << "running optimization";

    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);
    
    Log().msg() << summary.BriefReport();

}


/**
 * Loads the calibration data from config.dataDir and performs the sampling.
 */
bool Transfer::loadImpactData ()
{
    Log().msg() << "loading calibration data from " << config.dataDir << "/";
    
    ImageSource::params sConfig;
    sConfig.imagePath = config.dataDir;
    ImageSource isource (sConfig);
    
    timespec tstart, tend;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
    
    // setup the lightprobe that was used to capture the images
    Lightprobe::params caliConfig;
    caliConfig.load( config.dataDir+"/lightprobe.cfg" );
    
    // all pixels: use transfer probe mask for cali probe, too
    if (probe->getSamplingConfig().samplingMode == Lightprobe::samplingParams::ALLPIXELS) {
		caliConfig.maskFile = probe->getConfig().maskFile;
	}
    
    // check lowest and unify sampling angle of both probes
    if (probe->getConfig().horizonAngle > caliConfig.horizonAngle) {
        Log().msg() << "warning: specified sampling area is bigger than what the capture probe is capable of";
    } else {
        caliConfig.horizonAngle = probe->getConfig().horizonAngle;
    }    
    caliProbe = new Lightprobe ((Source*)&isource, caliConfig, probe->getSamplingConfig(), probe->samplingDirs);
    
    
    // assert: probes should match when doing all_pixel sampling 
    if (probe->getSamplingConfig().samplingMode == Lightprobe::samplingParams::ALLPIXELS) {
		cout << caliProbe->allDirs.size() << " " <<  probe->allDirs.size() << endl;
        assert (isource.getWidth() == probe->getSource()->getWidth());
        assert (isource.getHeight() == probe->getSource()->getHeight());
        assert(caliProbe->allDirs.size() == probe->allDirs.size());
    }
    
    
    imglib::Image<float> imgSum;
    
    Log().msg() << "calculating light impact from calibration images";
    
    
    // get darkframe
    isource.acquire();
    imglib::Image<float> darkFrame (isource.getImage());
    
    // load all impact images, calc imgsum 
    impactImages.clear();
    for (int l=0; l<numLamps; ++l) {
        
        isource.acquire();
        impactImages.push_back (isource.getImage());
        
        // sum up images
        if (l==0) { 
            imgSum=imglib::Image<float>(impactImages[0]); 
        } else { 
            imgAdd(imgSum, impactImages[l]);
        }
        cout << ".";
    }
    cout << endl;
    
    // calculate avereage brightness
    double avg=0;
    double numSamples=0;
    for (int x=0; x<imgSum.getWidth(); x++)
        for (int y=0; y<imgSum.getHeight(); y++)
            for (int c=0; c<imgSum.getNumChannels(); c++) {
                avg += imgSum(x,y,c);
                numSamples++;
            }
    averageBrightness = avg/(double)numSamples;
    
    Log().msg() << "average brightness is " << averageBrightness;
    
    if (not config.useAverageScale) averageBrightness = 1.0;
    
    // get maximum possible light impact for each sampling direction
    maximumImpacts = caliProbe->getImpact(imgSum);
    
    // save (scaled) max image
    imgMul(imgSum, 1/(double)numLamps);
    imgSum.save(config.dataDir + "/combined.ppm");   
      
    // calculate impacts  
    for (int l=0; l<numLamps; ++l) {
        lightImpacts.push_back(caliProbe->getImpact ( impactImages[l] ) );
        cout << ".";
    }
    cout << endl;
    
    
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
    Log().msg() << " loading calibration took " << ((double)tend.tv_nsec/1e9 + (double)tend.tv_sec) - ((double)tstart.tv_nsec/1e9 + (double)tstart.tv_sec) << " seconds";
    
    return true;
}


/**
 * Experimental: was used to analyze the reconstruction quality of the Gaussian sampling. 
 * Reconstructs a white image using the Sampling datastructure and dumps the image as well as values of horizontal slices.
 */
void Transfer::exp_plot_kernel(vector<dirCone> cones ) 
{
    imglib::Image<float> res(width, height, 1);
    for (int x=0; x<width; x++)
        for (int y=0; y<height; y++) {
            res(x,y,0) = 0; }
    
    for (int d=0; d<cones.size(); d++) {
        if (not caliProbe->usedDirections[d] || not probe->usedDirections[d]) continue;
        for (int p=0; p<cones[d].pixels.size(); p++) {
            int x=cones[d].pixels[p](0);
            int y=cones[d].pixels[p](1);
            res(x,y,0) += cones[d].weights[p];
        }
    }
     
    ofstream out;
    std::stringstream outfile;
    outfile << config.output << "_kernel.plot";
    out.open(outfile.str().c_str(), ios::out);
    for (int i=1; i<20; i++) {
        int y=(int)( (double)height/20.0 * (double)i);
        for (int x=0; x<width; x++) {
            out << x << " " <<  y << " " << res(x,y,0) << endl;
        }
        
    }
    out.close();
    
    res = imgScale(res);
    outfile.clear();
    outfile << config.output << "_kernel.pgm";
    res.save(outfile.str().c_str());
}


