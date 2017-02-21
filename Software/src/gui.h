/** 
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * The graphic user interface. Uses OpenGL via the glfw abstraction layer.
 *
 */

#ifndef GUI_HH
#define GUI_HH


#include "utils.h"
#include "lamps.h"
#include "sticks.h"
#include "image.h"
#include "source.h"

#include <unistd.h>

#include "GL/glfw.h"
#include <GL/glu.h>

//! The user interface.
class Gui {
    
  public:    
	
	Gui() {};
	Gui (int width, int height);
	~Gui();
    
    
    void start (int width, int height);
    
    // helper functions for 2D drawing
    const void drawLine (point f, point t);
	const void drawLine (int x1, int y1, int x2, int y2);
    const void drawBox (rect box);
    const void drawBox (int x1, int y1, int x2, int y2);
	const void drawCircle (circle c);
    const void drawCircle (circle c, int x, int y);
    const void drawCircleFilled (int xpos, int ypos, int radius);
	const void drawPoints (vector<Vector2i>);
	const void drawPoints (vector<Vector2i>, int x, int y);
	const void drawImage (imglib::Image<float>&, int, int);
    const void drawImage (imglib::Image<float>& img, int xpos, int ypos, bool isLinear);
    const void drawCircularImage (imglib::Image<float>& img, circle, int x, int y);
	const void drawCircularImage (imglib::Image<float>&, circle);
          
	const void drawStickRGBGrid (Lamps*, int x, int y, int w, int h, int space, bool);
    const void drawMonochromeLamps (Lamps* lamps, int x, int y, int box_width, int box_height, int space, bool vertical);
	
	// misc gui functions
	const circle runSphereSelection (Source*);
	const point runPixelSelection (Source*);
	
};

#endif
