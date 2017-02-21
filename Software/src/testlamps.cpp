/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Old class for testing and debugging the lamps. 
 *
 */

#include "testlamps.h"

TestLamps::~TestLamps(){};

void TestLamps::run() {


    LampPool* pool = dynamic_cast<LampPool*>(lamps);
    Sticks* sticks;
    if (pool == NULL) {
		sticks = dynamic_cast<Sticks*>(lamps);
        
    } else  {
		sticks = dynamic_cast<Sticks*>(pool->getMember(0));
	}
    
	
    Log().msg() << "lamp testing started";
	float v=1;
	int segsPerStick = 120/sticks->getConfig().segmentSize;
	
	sticks->start();
	
    imglib::Image<float> img;
    
    int stickLength = sticks->getStickLength(0);
    int width = source->getWidth();
    int height = source->getHeight();
    double etime = 1.0/30.0;
    
    bool flipVertical = true;       // flip over vertical axis
    bool flipHorizontal = false;    // flip over horizontal axis

    int testMode = 2;
    
	Gui gui (width + 20, height+100+30);
    
    double rgbStep = 0.01;
    rgb color = rgb(1,1,1);
    
    int testLampID = 0, lastTestLampID = 0;
    
    double lastTime, delTime;
    
	while (true) {
        lastTime = glfwGetTime();
        
        // clear window
        glClear(GL_COLOR_BUFFER_BIT);
        
		source->acquire();
		img = source->getImage();
        
        switch (testMode) {
            case 0:
                // sample input image on regular grid and map to lamp values
                // horizontal : sticks
                // vertical : lamp on stick (bottom up)
                for (int s=0; s<sticks->getNumSticks(); s++) {
                    for (int l=0; l<sticks->getStickLength(s); l++) {
                        int x=(int)( (double)width/(double)sticks->getNumSticks() * ((double)s+0.5) ); 
                        int y=(int)( (double)height/(double)sticks->getStickLength(s) * ((double)l+0.5) );
                        
                        
                        if (flipVertical) { y = height - y; }
                        if (flipHorizontal) { x = width - x; }
                        sticks->setStickRGBValue(s, l, sampleGauss7(img, x, y));
                    }
                }
                
                break;
                
            // test mode 2: cycle through all lamps (white light)
            case 1:
                if (glfwGetKey(GLFW_KEY_SPACE) == 1) {      // TODO _now_ the explicit keystate-query sucks...
                    
                    lastTestLampID = testLampID;
                    testLampID++;
                    if (testLampID >= sticks->getNumRGBLamps()) {
                        testLampID = 0;
                    }    
                    sticks->setRGBValue(lastTestLampID, rgb(0,0,0));
                    sticks->setRGBValue(testLampID, rgb(1,1,1));
                }
                break;
                
            // test mode 3: uniform lighting, change r,g,b with numpad (7/1) (8/2) (9/3) for (up/down)
            case 2:
                if (glfwGetKey(GLFW_KEY_KP_7) == 1) { color.r = (color.r<1-rgbStep)?color.r+rgbStep:1; }
                if (glfwGetKey(GLFW_KEY_KP_1) == 1) { color.r = (color.r>rgbStep)?color.r-rgbStep:0; }
                
                if (glfwGetKey(GLFW_KEY_KP_8) == 1) { color.g = (color.g<1-rgbStep)?color.g+rgbStep:1; }
                if (glfwGetKey(GLFW_KEY_KP_2) == 1) { color.g = (color.g>rgbStep)?color.g-rgbStep:0; }
                
                if (glfwGetKey(GLFW_KEY_KP_9) == 1) { color.b = (color.b<1-rgbStep)?color.b+rgbStep:1; }
                if (glfwGetKey(GLFW_KEY_KP_3) == 1) { color.b = (color.b>rgbStep)?color.b-rgbStep:0; }
               
                sticks->setAllRGB(color);
                Log().msg() << color.r << " " << color.g << " " << color.b;
                break;
        }
        
        // wait for next frame (if needed)
        delTime = glfwGetTime() - lastTime;
        if (delTime < etime) {
            usleep((long)(( etime - delTime)*1000000));
        }
        
        // update title to display max possible FPS
        std::stringstream title;
        title << "ALT  ( " << (1/delTime) << " max FPS)";
        glfwSetWindowTitle(title.str().c_str());
        
        
        // update GUI
        gui.drawImage(img,10,10);
        gui.drawStickRGBGrid (sticks, 10, 20+height, width / stickLength, 100/8, 0, true);
        glfwSwapBuffers();
        
        
        // handle user input
        glfwPollEvents();
        if (glfwGetKey(GLFW_KEY_ESC) == 1) {
            Log().msg() << "user aborted!";
            break;
        } else {
            if (glfwGetKey(GLFW_KEY_F1) == 1) {
                 //testMode = 0;
                 // not implemented for demo
            } else if (glfwGetKey(GLFW_KEY_F2) == 1) {
                 testMode = 1;
                 sticks->setAllRGB(rgb(0,0,0));
                 sticks->setRGBValue(0,rgb(1,1,1));
                 testLampID=0;
                    
            } else if (glfwGetKey(GLFW_KEY_F3) == 1) {
                 testMode = 2;
                 sticks->setAllRGB(rgb(0,0,0));
                 
            }
        }
        
    }
    
    sticks->stop();
    sleep(1);
}
