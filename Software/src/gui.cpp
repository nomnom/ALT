/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * The graphic user interface. Uses OpenGL via the glfw abstraction layer.
 *
 */
 
#include "gui.h"

using namespace std;

Gui::Gui (int width, int height)
{
	start (width, height);
}

Gui::~Gui() 
{
    glfwCloseWindow();
    glfwTerminate();
}


/**
 * Start the GUI with a specified window size. Sets up OpenGL and displays the window.
 * 
 * @param width Width of window in pixel.
 * @param height Height of window in pixel.
 */
void Gui::start (int width, int height)
{

    // init glfw and create window
    if (!glfwInit()) { 
        Log().err() << "Failed to initialize GLFW";
    }
    if (!glfwOpenWindow(width, height, 8,8,8, 0,8,0, GLFW_WINDOW )) {
    //if (!(window = glfwCreateWindow(width,height, GLFW`_WINDOWED, "ALT", NULL )) ) { // <-- old glfw method
        Log().err() << "Failed to open GLFW window";
    }
    
    glfwEnable(GLFW_STICKY_KEYS);
    
    glfwSetWindowTitle("A.L.T.");
            
    // setup OpenGL to 2d projection
    //glfwMakeContextCurrent(window);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
  
    // displacement trick for exact pixelization
    glTranslatef(0.375f, 0.375f, 0.0f);

    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}


/**
 *  Displays images acquired from source and asks user to select a sphere by clicking three points on the border of a circle.
 *  @param source Image source.
 *  @return The parameters of the circle.
 */
const circle Gui::runSphereSelection (Source* source)
{
    circle area;
    
    double fps=25; double lastTime, delTime;
    vector<Vector2d> points;
    bool clickedFlag = false;
    int xcur, ycur; 
    
    // main loop
    while (glfwGetKey(GLFW_KEY_ESC) != 1) {
        lastTime = glfwGetTime();
        
        // get input frame
        if (source->hasNewData()) {
            source->acquire();
        }
        imglib::Image<float> img = source->getImage();
        
        // process user input
        glfwGetMousePos (&xcur, &ycur);        
        if (!clickedFlag && glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) )  {
            points.push_back (Vector2d(xcur,ycur));
            clickedFlag=true;
            
            // calculate circumcenter, radius and return
            if (points.size() == 3) {
                
                area = getCircumCircle (points[0], points[1], points[2]);
                Log().msg() << "circle coords are \"" << area.x << " " << area.y << " " << area.r << "\"";
                
                // note: returns in button-up routine
            }
           
        
        } else if (clickedFlag && !glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT)) {
            clickedFlag=false;
            
            // return _after_ button-up 
            if (points.size() == 3) return area;
        }
        
        // update gui image
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        drawImage(img,0,0);
        
        // draw interactive circumcircle iff 2 points are specified
        if (points.size() == 2) {
            glColor3f(0,1,0);
            drawCircle ( getCircumCircle(points[0], points[1], Vector2d(xcur,ycur)));
        }
        
        // draw selected points
        glColor3f(1,0,0);
        for (int p=0; p<points.size(); p++) {
            drawLine (points[p][0]-7,points[p][1],points[p][0]+7,points[p][1]);
            drawLine (points[p][0],points[p][1]-7,points[p][0],points[p][1]+7);
        }
        
        // draw current cursor
        glColor3f(0,0,1);
        drawLine (xcur-7,ycur,xcur+7,ycur);
        drawLine (xcur,ycur-7,xcur,ycur+7);
        glfwSwapBuffers();    
        
        // framewait if needed
        delTime = glfwGetTime() - lastTime;
        if (delTime < 1/fps) {
            usleep((long)(( 1/(double)fps - delTime)*1000000));
        }
        glfwPollEvents();
    }
    return area;
}



 /**
 *  Displays images acquired from source and asks user to select a pixel / 2D coordinate.
 *  @param source Image source.
 *  @return The pixel position.
 */
const point Gui::runPixelSelection (Source* source)
{
    
    point pixel;
    double fps=25; double lastTime, delTime;
    vector<Vector2d> points;
    int xcur, ycur;
    bool clickedFlag = false;
    
    
    // main loop
    while (glfwGetKey(GLFW_KEY_ESC) != 1) {
        
        lastTime = glfwGetTime();
        
        // get input frame
        if (source->hasNewData()) {
            source->acquire();
        }
        imglib::Image<float> img = source->getImage();
        
        // process user input
        glfwPollEvents();
        glfwGetMousePos (&xcur, &ycur);        
        if (!clickedFlag && glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) )  {
            pixel.x = xcur; pixel.y = ycur;
            clickedFlag=true;
        } else if (clickedFlag && !glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) )  {
            clickedFlag=false;
            return pixel;
        }
        
        // update gui image
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        drawImage(img,0,0);
        
        // draw current cursor
        glColor3f(0,0,1);
        drawLine (xcur-3,ycur,xcur+3,ycur);
        drawLine (xcur,ycur-3,xcur,ycur+3);
        glfwSwapBuffers();    
        
        // display color values as window title
        std::stringstream title;
        title << "ALT";
        if (xcur >= 0 && xcur < img.getWidth() && ycur >= 0 && ycur < img.getHeight())
          title << " | color = " << (int)(img(xcur,ycur,0)*255) << "," << (int)(img(xcur,ycur,1)*255) << "," << (int)(img(xcur,ycur,2)*255);
        glfwSetWindowTitle(title.str().c_str());
        
        // framewait if needed
        delTime = glfwGetTime() - lastTime;
        if (delTime < 1/fps) {
            usleep((long)(( 1/(double)fps - delTime)*1000000));
        }
    }
    return pixel;
}


//////////  GUI drawing helpers ////////////////////////////////////

const void Gui::drawLine (point f, point t) { drawLine(f.x, f.y, t.x, t.y); }
const void Gui::drawLine (int x1, int y1, int x2, int y2)
{
    glBegin(GL_LINES);  
      glVertex2i(x1,y1);
      glVertex2i(x2,y2);
    glEnd();
}


const void Gui::drawBox (rect box) { drawBox(box.x, box.y, box.x+box.w, box.y+box.h); }
const void Gui::drawBox (int x1, int y1, int x2, int y2)
{
    glBegin(GL_QUADS);  
      glVertex2i(x1,y1);
      glVertex2i(x1,y2);
      glVertex2i(x2,y2);
      glVertex2i(x2,y1);
    glEnd();
}

const void Gui::drawCircle (circle c) { drawCircle(c,0,0); }
const void Gui::drawCircle (circle c, int x, int y)
{
    float deg2rad = 3.1415926f/180.0f;
    glBegin(GL_LINES); 
      for (int phi=0; phi<360; phi+=5) {
        glVertex2f(c.x+x+(c.r*cosf((float)phi*deg2rad)), c.y+y+(c.r*sinf((float)phi*deg2rad)));
      }
    glEnd();
}

const void Gui::drawCircleFilled (int xpos, int ypos, int radius)
{
    
    glBegin(GL_POINTS);     
    for (int x=-radius; x<radius; x++) { 
        for (int y=-radius; y<radius; y++) {
            if ( x*x+y*y < radius*radius ) {
                glVertex2i(xpos+x, ypos+y);
            }
        }
    }
    glEnd();    
}

const void Gui::drawImage (imglib::Image<float>& img, int xpos, int ypos, bool isLinear)
{
    int width = img.getWidth(), height = img.getHeight();
    glBegin(GL_POINTS);     
    
    if (isLinear) {
         
        for (int x=0; x<width; x++) { 
            for (int y=0; y<height; y++) {
                glColor3f(rgb2srgb_component(img(x,y,0)),
                          rgb2srgb_component(img(x,y,1)),
                          rgb2srgb_component(img(x,y,2)) );
                glVertex2i(x+xpos,y+ypos);
            }
        }
    } else {
        for (int x=0; x<width; x++) { 
            for (int y=0; y<height; y++) {
                glColor3f(img(x,y,0), img(x,y,1), img(x,y,2) );
                glVertex2i(x+xpos,y+ypos);
            }
        }
    }
    glEnd();  
    
}

const void Gui::drawImage (imglib::Image<float>& img, int xpos, int ypos)
{
     return drawImage (img, xpos, ypos, false);
}



const void Gui::drawCircularImage (imglib::Image<float>& img, circle area) { return drawCircularImage(img,area,0,0); }
const void Gui::drawCircularImage (imglib::Image<float>& img, circle area, int x_off, int y_off)
{
    int width = img.getWidth(), height = img.getHeight();
    
    glBegin(GL_POINTS);     
    for (int x=0; x<width; x++) { 
        for (int y=0; y<height; y++) {
            if (sqrt( (area.x-x)*(area.x-x) + (area.y-y)*(area.y-y) ) < area.r) {
                glColor3f(img(x,y,0),img(x,y,1),img(x,y,2));
                glVertex2i(x+x_off,y+y_off);
            }
        }
    }
    glEnd();    
}

const void Gui::drawPoints (vector<Vector2i> points) { drawPoints(points,0,0); }
const void Gui::drawPoints (vector<Vector2i> points, int x_off, int y_off ) { 
    
    glBegin(GL_POINTS);     
    for (int i=0; i<points.size(); i++) 
         glVertex2i( points[i](0) + x_off, points[i](1) + y_off);
    
    glEnd();     
}


/**
 *  Draw the lamp's RGB values as a table of rectangles
 *  @param x Top left position inside window.
 *  @param y Top left position inside window.
 *  @param box_width Width of rectangle.
 *  @param box_height Height of rectangle.
 *  @param space Spacing between rectangles.
 *  @param vertical Orientation
 * 
 */
const void Gui::drawStickRGBGrid (Lamps* lamps, int x, int y, int box_width, int box_height, int space, bool vertical)
{
    
    // requires lamps to be specialized to sticks class
    Sticks* sticks = dynamic_cast<Sticks*>(lamps);
    if (sticks == NULL) { 
        Log().err() << "drawStickRGBGrid can only work with Sticks";
        return;
    }
    
    
    glBegin(GL_QUADS);
    
    // draw all rectangles
    if (vertical) {
        for (int s=0; s<sticks->getNumSticks(); s++) {
            for (int l=0; l<sticks->getStickLength(s); l++) {
                rgb color = rgb2srgb(sticks->getStickRGBValue(s,l));
                glColor3f(color.r, color.g, color.b);
                drawBox(x+l*(box_width+space),
                        y+s*(box_height+space),
                        x+l*(box_width+space)+box_width,
                        y+s*(box_height+space)+box_height);
            }
        }
    } else {
        for (int s=0; s<sticks->getNumSticks(); s++) {
            for (int l=0; l<sticks->getStickLength(s); l++) {
                rgb color = rgb2srgb(sticks->getStickRGBValue(s,l));
                glColor3f(color.r, color.g, color.b);
                drawBox(x+s*(box_width+space),
                        y+l*(box_height+space),
                        x+s*(box_width+space)+box_width,
                        y+l*(box_height+space)+box_height);
            }
        }
    }
    
    glEnd();
}


/**
 *  Draw value of monochrome lamps as white rectangle.
 */
const void Gui::drawMonochromeLamps (Lamps* lamps, int x, int y, int box_width, int box_height, int space, bool vertical) {
    
    glBegin(GL_QUADS);
    
    if (vertical) {
        for (int l=0; l<lamps->getNumLamps(); l++) {
            glColor3f(lamps->getValue(l), lamps->getValue(l), lamps->getValue(l));
            drawBox(x+l*(box_width+space),
                    y,
                    x+l*(box_width+space)+box_width,
                    y+box_height);
        }
    } else {
        for (int l=0; l<lamps->getNumLamps(); l++) {
            glColor3f(lamps->getValue(l), lamps->getValue(l), lamps->getValue(l));
            drawBox(x,
                    y+l*(box_height+space),
                    x+box_width,
                    y+l*(box_height+space)+box_height);
        }
    }
    
    glEnd();
}
