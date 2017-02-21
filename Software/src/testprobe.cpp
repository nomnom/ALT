/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Old class for testing and debugging the probe. 
 *
 */


#include "testprobe.h"

TestProbe::~TestProbe(){};

void TestProbe::run()
{
    Log().msg() << "probe testing started";
	
    imglib::Image<float> img;
    
    int width = probe->getSource()->getWidth();
    int height = probe->getSource()->getHeight();
    
	Gui gui (max(200,width) + 20, height+60*3+20);
    circle sphereCircle = probe->getConfig().sphereCircle;
    
    int numBuckets = 200;
    
    double lastTime, delTime;
    double etime  = 1 / 25;
    
	while (glfwGetKey(GLFW_KEY_ESC) == 0) {
        lastTime = glfwGetTime();
        
        // clear window
        glClear(GL_COLOR_BUFFER_BIT);
        
        
        // acquire source image
        probe->acquire();
        img = probe->getImage();
        
        
        // calculate and display image histogram
        vector<int> buckets[3];
        int max[3];
        
        for (int c=0; c<3; ++c) {
            
            // fill buckets, use only area within the spherecircle
            buckets[c] = vector<int>(numBuckets);
            for (int y=0; y<height; ++y) {
                for (int x=0; x<width; ++x) {
                    if ((sphereCircle.x-x)*(sphereCircle.x-x) + (sphereCircle.y-y)*(sphereCircle.y-y) < sphereCircle.r*sphereCircle.r) {
                        buckets[c][(int)floor(img(x,y,c)*((double)numBuckets-1))] += 1;
                    }
                    
                }
            }
            // find max
            max[c] = 0;
            for (int b=0; b<numBuckets; ++b) { 
                if (buckets[c][b] > max[c]) { 
                    max[c] = buckets[c][b]; 
                }
            }
        }        
    
        int x_off = 10;
        int y_off = 20+height;
        int rowheight = 50;
        
        for (int c=0; c<3; ++c) {
            switch (c) {  default: case 0: glColor3f(1,0,0); break; case 1: glColor3f(0,1,0); break; case 2: glColor3f(0,0,1); break; }
            for (int b=0; b<numBuckets; ++b) {
                if (max[c] > 0) 
                    gui.drawLine(x_off+b, y_off+c*(rowheight+10)+rowheight, 
                                 x_off+b, y_off+c*(rowheight+10)+rowheight-((double)rowheight/(double)max[c]*(double)buckets[c][b]));
            }
        }
             
        
        // update GUI
        gui.drawCircularImage(img,sphereCircle, 10,10);
        glfwSwapBuffers(); 
        
        
        // wait for next frame (if needed)
        delTime = glfwGetTime() - lastTime;
        if (delTime < etime) {
            usleep((long)(( etime - delTime)*1000000));
        }
        
        // update title to display max possible FPS
        std::stringstream title;
        title << "ALT  ( " << (1/delTime) << " max FPS)";
        glfwSetWindowTitle(title.str().c_str());
        
        
        // handle user input
        glfwPollEvents();
        
    }
    
    

}


