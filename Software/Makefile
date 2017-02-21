
#
#  Ambient Light Transfer makefile
#

#########################
#  
#  build application
#

ifeq ($(CC),clang)
 FLAGS = -g  
else
 FLAGS = -g  -Ofast
 CC = g++ 
endif

SRCS = $(wildcard src/*.cpp)
HDRS  = $(wildcard src/*.h)
OBJS = $(patsubst src/%.cpp,bin/%.o,$(SRCS))

#CERES_LIBS =  -L/usr/local/lib  -fopenmp -lm -lglog -lceres -lprotobuf -lgomp -llapack -lcamd -lamd -lccolamd -lcolamd -lcholmod -lcxsparse
CERES_LIBS =  -L/usr/local/lib -lceres  -fopenmp -lm -Wl,-rpath=/usr/local/lib -lglog -Wl,-rpath=/usr/local/lib -lgflags  -lprotobuf -lgomp -llapack -lcamd -lamd -lccolamd -lcolamd -lcholmod -lcxsparse
LIBS =   -Llib/ -Wl,-rpath=lib/ -lglfw -lGLU -lGL  -lX11  -lXext -lpthread ${CERES_LIBS} -lpython2.7 -lrt
INCLUDES = -Iinc/ -I/usr/include/eigen3/  -Iinc/cvxopt/C  -I/usr/include/python2.7 

all: alt

alt: $(OBJS)
	${CC} ${FLAGS} -o alt $(OBJS)  $(LIBS)

bin/%.o: src/%.cpp src/%.h
	${CC} ${FLAGS} -o $@ -c $< $(INCLUDES)


tools/imgtools: src/imgtools.c
	${CC} ${FLAGS} -o bin/imgtools.o  -c src/imgtools.c
	${CC} ${FLAGS} -o tools/imgtools bin/imgtools.o 
	
clean:
	rm  $(patsubst src/%.cpp,bin/%.o,$(SRCS))

cleanall:
	rm  $(OBJS)


# build doxygen config
doxy:
	doxygen doxygen_config


#######################
#
#  run ALT software  (controlled by script in ../Results)
#

#  configuration
ROOM=plab_8x40i
STICKS=
CDATA=data/rooms/$(ROOM)
ODATA=data/objectives


# run room calibration
capture:
	sh tools/capture_impacts.sh 
	sh tools/capture_impacts_convert.sh `pwd`/cdata/$ROOM
	

# use gauss
GAUSS_ARG=
ifdef USE_GAUSS
  GAUSS_ARG=-s 0.5
endif


#direction config
DIRS_64 = -m 50 -n 64 -i data/sampling/fliege_64.vecs
DIRS_121 = -m 50 -n 121 -i data/sampling/fliege_121.vecs 
DIRS_256 = -m 50 -n 256 -i data/sampling/fliege_256.vecs -c 0.30 $(GAUSS_ARG)
DIRS_400 = -m 50 -n 400 -i data/sampling/fliege_400.vecs
DIRS_576 = -m 100 -n 576 -i data/sampling/fliege_576.vecs
DIRS_900 = -m 50 -n 900 -i data/sampling/fliege_900.vecs

D=256
ifeq ($(D),64)
  DIRS=$(DIRS_64)
endif
ifeq ($(D),121)
  DIRS=$(DIRS_121)
endif
ifeq ($(D),256)
  DIRS=$(DIRS_256)
endif
ifeq ($(D),400)
  DIRS=$(DIRS_400)
endif
ifeq ($(D),576)
  DIRS=$(DIRS_576)
endif
ifeq ($(D),900)
  DIRS=$(DIRS_900)
endif
SAMPLING_ARG= --sample_file $(DIRS) 

ALL_ARG=
ifeq ($(D),all)
  SAMPLING_ARG= --sample_all
  ALL_ARG=_all
endif

# dump results if required
ODIR_ARG=
ifdef ODIR
   ODIR_ARG= -o $(ODIR) 
endif

# capture photo if required
EXEC_ARG=
ifdef DO_CAPTURE
   EXEC_ARG= -e 
endif

ifdef DO_RGB_CAPTURE
   EXEC_ARG= -3
endif


# num iterations
ITER_ARG=
ifdef S
   ITER_ARG= -c $S 
endif


# num repeats of sampling and algorithm
REPS_ARG=
ifdef R
   REPS_ARG= -n $R
endif

#uniform color requires special argument
UNIFORM_ARG=
ifeq ($(findstring uni, $(ROOM)),uni)
   UNIFORM_ARG= -u true 
endif

# set target scale
SCALE_ARG=
ifdef SCALE
   SCALE_ARG = -s $(SCALE)
endif

ifdef REFSCALE
   SCALE_ARG = -g -s $(REFSCALE)
endif

ifdef RAMPSCALE
   SCALE_ARG = -a '$(RAMPSCALE)'
endif


#construct alt argument list
ALT=./alt --sticks -d /dev/ttyACM0 -f0 --transfer -i $(CDATA) -r30 $(ODIR_ARG) $(EXEC_ARG) $(SCALE_ARG) $(ITER_ARG) $(REPS_ARG) $(UNIFORM_ARG) $(SAMPLING_ARG) 


#
# the IMPORTANT targets used by result scripts
#

# run default transfer from $TARGET directory
run_dir: alt
	$(ALT) --imagesource -i $(ODATA)/$(TARGET)

# transfer captured env maps (each with own lightprobe config file)
run_env: alt
	$(ALT) --imagesource -i $(ODATA)/envmaps/$(TARGET).ppm --probe -l $(ODATA)/envmaps/$(TARGET)$(ALL_ARG).cfg

# transfer simple handdrawn env maps (all with the same lightprobe config file)
run_simple: alt
	$(ALT) --imagesource -i $(ODATA)/simple/$(TARGET).ppm 

# grab from x11
run_x11:	alt	
	$(ALT) --x11source -x "$(DISPLAY)" -a "30 30 500 500" --probe -l $(CDATA)/lightprobe.cfg -w "1 1 1" -o linear -h 0
		  
# transfer debevec env maps 
run_debevec:  alt
	$(ALT) --probe -l $(ODATA)/debevec/lightprobe.cfg -h 0 -t 1 -o linear --imagesource -i $(ODATA)/debevec/$(TARGET)_probe.ppm
	

.PHONY: all clean cleanall
