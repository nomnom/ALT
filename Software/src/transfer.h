/**
 * @author Manuel Jerger <nom@nomnom.de>
 * 
 * Implements the Ambient Light Transfer loop.
 *
 */


#ifndef TRANSFER_HH
#define TRANSFER_HH

#include "utils.h"
#include "gui.h"
#include "lamps.h"
#include "lamppool.h"
#include "virtuallamps.h"
#include "source.h"
#include "imagesource.h"
#include "lightprobe.h"
#include "image.h"

#include <vector>
#include <time.h>

// GLFW / OpenGL 
#include "GL/glfw.h"
#include <GL/glu.h>


// for ceres
#include <ceres/ceres.h>
#include <glog/logging.h>

// cvxopt
#include "cvxopt.h"

using namespace std;

//! The Ambient Light Transfer loop.
class Transfer {
    
  public:
  
    //! Configuration
    struct params {
        params() : rate(30), dataDir(""), algorithm(OPT), numIterations(0), targetScale(0.25), output(""),
        rampScaleFrom(0), rampScaleTo(0), rampScaleSteps(1), dynamicFading(false), useAverageScale(false), exec(""), useUniform(false), numRepeats(1), driveSeparateColors(false) {}
        
        enum { OPT, SAMPLER } algorithm;
        double rate;
        string dataDir;
        int mode;
        string output;
        double targetScale;
        double rampScaleFrom;
        double rampScaleTo;
        double rampScaleSteps;
        bool dynamicFading;
        string exec;
        bool useUniform;
        int numIterations;      //number of cycles for ALT loop
        bool useAverageScale;
        int numRepeats;          // repeats of sampling / optimization algo (for runtime averaging)
        bool driveSeparateColors;
    };
  
    Transfer (Lightprobe* p, Lamps* l, params c);
    ~Transfer();

    params getConfig();

    void run();
    void repaint();
    
    

    void exp_plot_kernel(vector<dirCone> cones );
    
  private:
  
    params config;

    Lightprobe* probe;      // lightprobe used for capturing target impacts
    Lightprobe* caliProbe;  // calibration probe
    Lamps* lamps;
    
    Gui* gui;
    
    int numLamps;
    int numDirs;
    int numSamples;
    
    int width, height;
    int width_cali, height_cali;
    
    void createResults(int);
    
    bool loadImpactData ();     // load impact images and precalculate light impacts
    void prepareDataCeres();
    void runCeres();
    
    void prepareDataCVXOPT();
    void runCVXOPT();
    
    // precalculated data
    vector<imglib::Image<float> > impactImages;
    vector<rgb> maximumImpacts;
    vector<vector<rgb> > lightImpacts;
    vector<int> samplingDirectionsNearestPixel;
    double averageBrightness;
    
    // runtime data for optimization
    imglib::Image<float> targetImage;
    vector<rgb> targetImpact;
    
    //vector<bool> unusedDirections;
    
    // runtime options
    bool scaleImpact;
    bool lowPrecision;  //ceres
    bool resetWeights;  //ceres
    double targetScale;
    // ceres
    double* weights;
    double* targetData;
    double* impactData;   // memory layout is like arr[colorIndex][dirIndex][lampIndex]
 
 
    // cvxopt
    PyObject *qpsolver;
    PyObject *qpsolverArgs;
    PyObject *qp_c;
    PyObject *qp_Q;
    PyObject *qp_A;
    PyObject *qp_b;
    
    // gui draw options
    bool drawTarget;
    int drawSamplingCones;  // 1=random color 2= sampled color
    bool drawPseudoResult;
    bool drawPseudoResultCones;
    bool drawDifference;
    bool doAutoAdjust;
    double drawScalingFactor;
    int keyPressFlag;
    
    

    // helpers
    bool toggleByKey(bool var, int key);
    
//#define USE_ALL_PIXELS    
    
#define PENALTY 100
#define NUMLAMPS 108
//#define NUMSAMPLES (60*3)

// quadratic or linear penalty for residual box constraint
//#define RESIDUAL_SQUARED_PENALTY

// use line search or trust region
//#define CERES_LINE_SEARCH

//#define CERES_JACOBI
 
// break with infinite costs or use penaly function
//#define CERES_BREAK_WITH_INF

//#define USE_CERES

  // CERES
    // functor for ceres (DynamicAutoDiffCostFunction) 
    // we use one residual per light direction
    //! CostFunction for ceres.
    struct Residual {
        Residual (double _target, int _size, double* _data) : target(_target), size(_size), data(_data) {} 
        const double target;  // goal
        const int size;        // number of weights (= # monochrome lamps)
        const double* data;    // impacts of lamps
        template <typename T>
        bool operator ()(const T* const  weights, T* residuals) const {
            //bool operator ()(double const* const*  weights, double* residuals, double** jacobian) const {
            
            residuals[0] = T(-target);
            for (int i=0; i<size; ++i) {
				
            /*     // clamp positive
                 if (weights[i] > T(0.0) ) {
                    residuals[0] += weights[i] * T(data[i]);
				 }*/
           
                
           /*
                
                
              // clamp to 0..1
                if (weights[i] > T(0.0) && weights[i] <= T(1.0)) {
                   residuals[0] += weights[i] * T(data[i]);
                } else if (weights[i] > T(1.0)) {
					residuals[0] += T(data[i]);
				}
				
*/
              
           
               // with penalty for <0 and >1
                if (weights[i] >= T(0.0) &&  weights[i] <= T(1.0)) {
                    residuals[0] += weights[i] * T(data[i]);
#ifdef CERES_BREAK_WITH_INF
                } else {
                    return false;
                }
#else                
  #ifdef RESIDUAL_SQUARED_PENALTY                    
                } else if (weights[i] < T(0.0)) {
                    residuals[0] += weights[i]*weights[i]*T(data[i])*T(PENALTY);
                } else if (weights[i] > T(1.0)) {
                    residuals[0] += (T(1)-weights[i])*(T(1)-weights[i])*T(data[i])*T(PENALTY);
  #else
                } else if (weights[i] < T(0.0)) {
                    residuals[0] += weights[i]*T(-1)*T(data[i])*T(PENALTY);
                } else if (weights[i] > T(1.0)) {
                    residuals[0] += (weights[i]-T(1))*T(data[i])*T(PENALTY);
  #endif                    
                }
#endif
					
                    
                
            }
            
            return true;     
        }
    };   
    
    
    // same as above commented code, but optimized for less condition evaluations
    //! Faster CostFunction for ceres.
    class CostSimple : public ceres::SizedCostFunction<1, NUMLAMPS> {
        public:
            ~CostSimple() {};
        virtual bool Evaluate(double const* const* parameters,
                              double* residuals,
                              double** jacobians) const
        {
            
                residuals[0] = -target;
            // calculate residual and jacobian
            if (jacobians != NULL && jacobians[0] != NULL) {
                for (int l=0; l<NUMLAMPS; ++l) {
                    
                      
                    // with penalty for <0 and >1
                    if (parameters[0][l] >= 0.0 &&  parameters[0][l] <= 1.0) {
                        residuals[0] += parameters[0][l] * samples[l];
                        jacobians[0][l] = samples[l];
#ifdef CERES_BREAK_WITH_INF
                } else {
                    return false;
                }
#else     

                    } else if (parameters[0][l] < 0.0) {
                        

  #ifdef RESIDUAL_SQUARED_PENALTY   
                        residuals[0] += parameters[0][l]*parameters[0][l]*samples[l]*PENALTY;      // w^2*PENALTY
                        jacobians[0][l] = 2*parameters[0][l]*samples[l]*PENALTY;
  #else
                        // linear
                        residuals[0] += -parameters[0][l]*(-samples[l])*PENALTY;      // w*PENALTY
                        jacobians[0][l] = -samples[l]*PENALTY; 
  #endif                    
                        
                    } else if (parameters[0][l] > 1.0) {
                        
                        
  #ifdef RESIDUAL_SQUARED_PENALTY   
                        residuals[0] += (1.0-parameters[0][l])*(1.0-parameters[0][l])*samples[l]*PENALTY;      // (1-w)^2 * PENALTY
                        jacobians[0][l] = (2*parameters[0][l]-2)*samples[l]*PENALTY;
  #else                        
                        // linear
                        residuals[0] += (parameters[0][l]-1)*samples[l]*PENALTY;      // (1-w) * PENALTY
                        jacobians[0][l] = samples[l]*PENALTY;
  #endif                        
                    }
#endif

                }
                
                
                
            // calculate only residual
            } else {
                
                for (int l=0; l<NUMLAMPS; ++l) {
                    
                    // with penalty for <0 and >1
                    if (parameters[0][l] >= 0.0 &&  parameters[0][l] <= 1.0) {
                        residuals[0] += parameters[0][l] * samples[l];
                    } else if (parameters[0][l] < 0.0) {
#ifdef RESIDUAL_SQUARED_PENALTY                         
                        residuals[0] += parameters[0][l]*parameters[0][l]*samples[l]*PENALTY;     
#else                        
                        residuals[0] += parameters[0][l]*(-samples[l])*PENALTY; 
#endif                        
                    } else if (parameters[0][l] > 1.0) {
#ifdef RESIDUAL_SQUARED_PENALTY                         
                        residuals[0] += (1.0-parameters[0][l])*(1.0-parameters[0][l])*samples[l]*PENALTY;   
#else                        
                        residuals[0] += (parameters[0][l]-1)*samples[l]*PENALTY;   
#endif                        
                        
                    }
                }
            }
                
            return true;
        }
        
        CostSimple (double target, double* samples) : target(target), samples(samples) {} ;
      private:
        double target;
        double* samples;
    }; 
    


};



#endif
