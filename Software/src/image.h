#ifndef IMAGE_HH
#define IMAGE_HH

/**
 * This file contains demonstration code for the lecture on Visual
 * Computing in the winter term 2011/2012 at Stuttgart University. 
 * Please do not distribute it.
 *
 * Martin Fuchs <martin.fuchs@visus.uni-stuttgart.de>
 */

#include <limits>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <stdexcept>

#include <cstring>

namespace imglib {

  /*  small helper functions *****************************************/

  /**
   * @returns true if and only if the string specified by
   * <code>search</code> ends in the character sequence specified by
   * <code>end</code>
   * @param search string to look in
   @ @param end string to look for
   */

  inline bool endswith(const char *search, const char *end) {
    int searchlen = strlen(search);
    int endlen = strlen(end);
    if (searchlen < endlen) {
      return false;
    }
    for (int i = 0; i < endlen; ++i) {
      if (search[i+searchlen-endlen] != end[i]) {
	return false;
      }
    }
    return true;
  }

  /**
   * peeks at a stream, reads and ignores any whitespace it finds.
   * @param instream istream to operate on
   */
  
  inline void munchWhitespace(std::istream& instream) {
    int charRead = instream.peek();
    while (charRead >= 0 && charRead <= ' ') {
      instream.ignore();
      charRead = instream.peek();
    }  
  }

  /**
   * peeks at a stream, reads and ignores whitespace. 
   * If the next char is a hash character '#', it is read
   * until after the next '\n' character or before an eof is reached. The process
   * is repeated, until the next character in the stream is not a hash character.
   * @param instream istream to operate on
   */
  
  inline void munchComments(std::istream& instream) {
    munchWhitespace(instream);
    int charRead = instream.peek();
    while (charRead >= 0 && charRead == '#') {
      while ((charRead != '\n') && (!instream.eof())) {      
	charRead = instream.get();
      }
      charRead = instream.peek();
    }  
  }

  /**
   * converts a string to any object
   * @param str string to convert
   * @return the converted object of type T
   */
  
  template <class T> inline T atot(const std::string &str) {
    std::istringstream s(str);
    T result;
    s >> result; 
    return result;
  }
  


 
  /* Header abstractions for P?M file formats ***********************/

  
  /**
   * @brief encodes a PPM header. Only raw encoding (P6) is supported.
   */
  
  template <class Scalar> class PPMHeader {
  public:
    
    /**
     * creates a PPM header for zero width, zero height, and zero maxVal values.
     */
    
    PPMHeader() : maxVal(0),
		  width(0),
		  height(0) {  
      
    }
    
    /**
     * creates a PPM header for given width, height, and maxVal values.
     */
    
    PPMHeader(int width, int height, int maxVal) : maxVal(maxVal),
						   width(width),
						   height(height) {  
    }
    
    /**
     * creates a PPM header read from 
     * @param in istream
     */
  
    PPMHeader(std::istream& in) {  
      read(in);
    }


    /**
     * reads a PPM file format header from a stream. The next char in
     * the stream will be raw input data.  
     * @param in istream 
     * @throws runtime_error if the stream is not in PPM format
     */

    void read(std::istream& in) {
      char P;
      in >> P;
      char five;
      in >> five;
      if (P != 'P' || five != '6') {
	throw std::runtime_error((std::string) "Error: input stream is not in raw PPM format -- wrong magic number"); 
      }
      
      munchComments(in);
      in >> width;
      munchComments(in);
      in >> height;
      munchComments(in);
      in >> maxVal;
      
      in.get(); // skip separating whitespace 
    }

    /**
     * writes PPM file format header to
     * @param out ostream
     */  
  
    void write(std::ostream& out) {
      out << "P6\n";
      out << width << ' ' << height << '\n' << maxVal << (char) (10); // not '\n', this may produce incorrect results in Windows if the file stream is not binary.
    
    }

    /**
     * reads the entire image into one vector of Scalars, rgb channels interleaved,
     * each Scalar ranging from 0 to 1.
     */

    std::vector<Scalar> readScalarVector(std::istream& in) {

      std::vector<Scalar> entries;
      entries.resize(width * height * 3);

      // read en bloc
  
      unsigned int numChars =  3 * width * height * (maxVal > 255 ? 2:1);
      unsigned char *buf = new unsigned char[numChars];
      unsigned int bufidx = 0;
      in.read((char*) buf,numChars);

      /// distribute and convert into Scalars
  
      if (maxVal < 256) {
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height * 3; ++i) {
	  entries[i] = buf[bufidx] / (Scalar) maxVal;
	  bufidx += 1;
	}
      } else { // maxVal >= 256
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height * 3; ++i) {
	  entries[i] = ((unsigned int) buf[bufidx] * (unsigned int) 256 + (unsigned int) buf[bufidx+1]) / (Scalar) maxVal;
	  bufidx += 2;
	}
      }
  
      delete[] buf;      
      return std::vector<Scalar> (entries);

    }

    void writeScalarVector(std::ostream& out, const std::vector<Scalar> &entries) {

      assert(entries.size() == (unsigned int) width * height * 3);
  
      // write en bloc
  
      unsigned int numChars =  3 * width * height * (maxVal > 255 ? 2:1);
      unsigned char *buf = new unsigned char[numChars];

      /// distribute and convert from Scalars

      int bufidx = 0;

      if (maxVal < 256) {
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height * 3; ++i) {
	  buf[bufidx] = (unsigned char) ((std::min(std::max(entries[i],0.f),1.f)  * (Scalar) maxVal));
	  bufidx += 1;
	}
      } else { // maxVal >= 256
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height * 3; ++i) {
	  unsigned int val = (unsigned int) ((std::min(std::max(entries[i],0.f),1.f)  * (Scalar) maxVal));

	  buf[bufidx] = val / 256;
	  buf[bufidx+1] = val % 256;
	  bufidx += 2;
	}
      }
  
      out.write((char*) buf,numChars);
      delete[] buf;      
    }

    /** the maximally encoded gray value, from
     * 0 to 65535 inclusive. If it is below 256, 
     * the pnm stream has a color depth of 8 bits, otherwise, of 16 bits per pixel
     */ 

    int maxVal;

    /**
     * the width of the ppm image
     */

    int width;

    /**
     * the height of the ppm image
     */

    int height;

  };

  /**
   * @brief encodes a PPM header. Only raw encoding (P5) is supported.
   *
   * @author Martin Fuchs
   *
   * This file was copied from brdfutils and moved to
   * FaceMeasurements/common. The old version is deprecated. Later, it
   * traveled to some space in the AG4 hierarchy, then to imglib/,
   * then to the visual computing lecture in Stuttgart ...
   */

  template <class Scalar> class PGMHeader {
  public:
    
    /**
     * creates a PPM header for zero width, zero height, and zero maxVal values.
     */
    
    PGMHeader() : maxVal(0),
		  width(0),
		  height(0) {  
      
    }
    
    /**
     * creates a PGM header for given width, height, and maxVal values.
     */
    
    PGMHeader(int width, int height, int maxVal) : maxVal(maxVal),
						   width(width),
						   height(height) {  
    }
    
    /**
     * creates a PGM header read from 
     * @param in istream
     */
    
    PGMHeader(std::istream& in) {  
      read(in);
    }
    

    /**
     * reads a PPM file format header from a stream. The next char in
     * the stream will be raw input data.  
     * @param in istream 
     * @throws runtime_error if the stream is not in PPM format
     */

    void read(std::istream& in) {
      char P;
      in >> P;
      char five;
      in >> five;
      if (P != 'P' || five != '5') {
	throw std::runtime_error((std::string) "Error: input stream is not in raw PGM format -- wrong magic number"); 
      }
      
      munchComments(in);
      in >> width;
      munchComments(in);
      in >> height;
      munchComments(in);
      in >> maxVal;
      
      in.get(); // skip separating whitespace 
    }

    /**
     * writes PPM file format header to
     * @param out ostream
     */  
  
    void write(std::ostream& out) {
      out << "P5\n";
      out << width << ' ' << height << '\n' << maxVal << (char) (10); // not '\n', this may produce incorrect results in Windows if the file stream is not binary.
    
    }

    /**
     * reads the entire image into one vector of Scalars, rgb channels interleaved,
     * each Scalar ranging from 0 to 1.
     */

    std::vector<Scalar> readScalarVector(std::istream& in) {

      std::vector<Scalar> entries;
      entries.resize(width * height);

      // read en bloc
  
      unsigned int numChars =  width * height * (maxVal > 255 ? 2:1);
      unsigned char *buf = new unsigned char[numChars];
      unsigned int bufidx = 0;
      in.read((char*) buf,numChars);

      /// distribute and convert into Scalars
  
      if (maxVal < 256) {
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height; ++i) {
	  entries[i] = buf[bufidx] / (Scalar) maxVal;
	  bufidx += 1;
	}
      } else { // maxVal >= 256
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height; ++i) {
	  entries[i] = ((unsigned int) buf[bufidx] * (unsigned int) 256 + (unsigned int) buf[bufidx+1]) / (Scalar) maxVal;
	  bufidx += 2;
	}
      }
  
      delete[] buf;      
      return std::vector<Scalar> (entries);
    }
  
    void writeScalarVector(std::ostream& out, const std::vector<Scalar> &entries) {
      assert(entries.size() == (unsigned int) width * height);
  
      // write en bloc
  
      unsigned int numChars =  width * height * (maxVal > 255 ? 2:1);
      unsigned char *buf = new unsigned char[numChars];

      /// distribute and convert from Scalars

      int bufidx = 0;

      if (maxVal < 256) {
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height; ++i) {
	  buf[bufidx] = (unsigned char) ((std::min(std::max(entries[i],0.f),1.f)  * (Scalar) maxVal));
	  bufidx += 1;
	}
      } else { // maxVal >= 256
	for (unsigned int i = 0; i < (unsigned int) width * (unsigned int) height; ++i) {
	  unsigned int val = (unsigned int) ((std::min(std::max(entries[i],0.f),1.f)  * (Scalar) maxVal));

	  buf[bufidx] = val / 256;
	  buf[bufidx+1] = val % 256;
	  bufidx += 2;
	}
      }
  
      out.write((char*) buf,numChars);
      delete[] buf;      

    }

    /** the maximally encoded gray value, from
     * 0 to 65535 inclusive. If it is below 256, 
     * the pnm stream has a color depth of 8 bits, otherwise, of 16 bits per pixel
     */ 

    int maxVal;

    /**
     * the width of the ppm image
     */

    int width;

    /**
     * the height of the ppm image
     */

    int height;

  };

  /**
   * @brief Header for floating point image files in PFM format.
   */

  template <class Scalar> class PFMHeader {
  public:
    
    /**
     * creates a PFM header for zero width, zero height, and a single channel.
     */

    PFMHeader() : numChannels(1),
		  width(0),
		  height(0) {  
    }

    /**
     * creates a PFM header for given width, height, and number of channels
     * (either 1 or 3) values.
     */
  
    PFMHeader(int width, int height, int numChannels) : numChannels(numChannels),
							width(width),
							height(height) {  
    }
    
    /**
     * creates a PFM header read from 
     * @param in istream
     */
  
    PFMHeader(std::istream& in) {  
      read(in);
    }
    
    /**
     * reads a PFM file format header from. The next char in the stream will be
     * raw input data.
     * @param in istream 
     * @throws runtime_error if the stream is not in PFM format
     */

    void read(std::istream& in) {
      char P;
      in >> P;
      char flag;
      in >> flag;
      if (P != 'P' || (flag != 'f' && flag != 'F') ) {
	throw std::runtime_error((std::string) "Error: input stream is not in raw PFM format -- wrong magic number"); 
      }
  
      if (flag == 'f') {
	numChannels = 1;
      } else {
	numChannels = 3;
      }

      munchComments(in);
      in >> width;
      munchComments(in);
      in >> height;
      munchComments(in);
      in >> scale;

      in.get(); // skip separating whitspace 
    }

    /**
     * writes PFM file format header to
     * @param out ostream
     */  
  
    void write(std::ostream& out) {
      if (numChannels == 1) {
	out << "Pf\n";
      } else {
	out << "PF\n";
      }
      out << width << ' ' << height << '\n' << -1 << (char) (10); 
      // 1 is for big endian      
    }

    /**
     * reads the entire image into one vector of Scalars, rgb channels interleaved,
     * each Scalar ranging from 0 to 1.
     */

    std::vector<Scalar> readScalarVector(std::istream& in) {
      
      std::vector<Scalar> entries;
      entries.resize(width * height * numChannels);

      // read en bloc
  
      unsigned int numChars =  numChannels * width * height * 4; // 4 ==
      //  sizeof(Scalar) in the format
      unsigned char *buf = new unsigned char[numChars];
      in.read((char*) buf,numChars);
  
      Scalar* bufferAsScalar = (Scalar*) buf;

      for (int i = 0; i < width*height*numChannels ; ++i) {
	entries[i] = bufferAsScalar[i] / fabsf(scale);
      }

      delete[] buf;      
      return std::vector<Scalar> (entries);

    }

    /**
     * writes one vector of floats
     */

    void writeScalarVector(std::ostream& out, const std::vector<Scalar> &entries) {
      assert(entries.size() == (unsigned int) width * height * numChannels);
  
      // write en bloc
  
      unsigned int numChars =  numChannels * width * height * 4; // 4 is the size of
      //  Scalar in the output format

      unsigned char *buf = new unsigned char[numChars];

      Scalar* bufferAsScalar = (Scalar*) buf;

      /// distribute and convert from Scalars

      for (int i = 0; i < width*height*numChannels ; ++i) {
	bufferAsScalar[i] = entries[i];
      }
  
      out.write((char*) buf,numChars);
      delete[] buf;      
    }

    /** number of channels. Either 1 or 3.
     */ 

    int numChannels;

    /**
     * the width of the pgm stream
     */

    int width;

    /**
     * the height of the pgm stream
     */

    int height;

    /**
     * in case the input is scaled. Negative implies little endian, positive is
     * big endian encoding.
     */

    float scale;

  };



  /* Main image class ***********************************************/

  /**
   * @brief basic image class: supports reading and writing PNM and PFM
   * files specified by filenames and streams, plus operator() access.
   */

  template <class Scalar> class Image {
  public:
  
    /** 
     * creates a new image of dimension 0 x 0 and 0 channels
     * 
     */

    Image() : width(0), height(0), numChannels(0) {
    }
  
    /** 
     * creates a new, uninitialized image of given dimension and channels
     * 
     * @param width 
     * @param height 
     * @param numChannels 
     */

    Image(int width, int height, int numChannels) : width(width),
						    height(height),
						    numChannels(numChannels),
						    d((size_t)width*height*numChannels) {
    }

    /**
     * creates an image from a pnm / pfm file in a stream, by peeking
     * at the header bytes and determining from there what to do.
     */

    Image(std::istream &in) {
      int firstChar = in.get();
      int secondChar = in.peek();
      
      if (!in.good() || in.eof()) {
	throw std::runtime_error ("Error: can't read image data from provided stream.");
      }
      
      in.unget();

      if (firstChar != 'P') {
	throw std::runtime_error ("Error: can't read image data from probided stream.");	
      }

      if (secondChar == '6') {
	/// load PPM file
      
	PPMHeader<Scalar> hdr(in);
	width  = hdr.width;
	height = hdr.height;
	numChannels = 3;
      
	d = hdr.readScalarVector(in);
	return;     
 	
      } else if (secondChar == '5') {
	/// load PGM file
      
	PGMHeader<Scalar> hdr(in);
	width  = hdr.width;
	height = hdr.height;
	numChannels = 1;
      
	d = hdr.readScalarVector(in);
	return;      	
	
      } else if (secondChar == 'f' || secondChar == 'F' ) {

	/// load PFM file
      
	PFMHeader<Scalar> hdr(in);
	width  = hdr.width;
	height = hdr.height;
	numChannels = hdr.numChannels;

	d = hdr.readScalarVector(in);
	return;      		
      } else {
	throw std::runtime_error ("Can't construct image from provided stream: no PGM, PPM, or PFM file contained within.");
      }
    }

    /**
     * creates an image from an ppm, pgm or pfm file, depending on
     * the file name suffix. 
     *
     * @param filename string with the file name of the exr file
     */
  
    Image(const std::string& filename) {
    
      if (endswith(filename.c_str(), ".pfm")) {
	/// load PPM file
      
	std::ifstream in(filename.c_str(), std::ios::binary);
	PFMHeader<Scalar> hdr(in);
	width  = hdr.width;
	height = hdr.height;
	numChannels = hdr.numChannels;
      
	d = hdr.readScalarVector(in);
	return;
      
      } else if (endswith(filename.c_str(), ".ppm")) {
	/// load PPM file
      
	std::ifstream in(filename.c_str(), std::ios::binary);
	PPMHeader<Scalar> hdr(in);
	width  = hdr.width;
	height = hdr.height;
	numChannels = 3;
      
	d = hdr.readScalarVector(in);
	return;
      
      } else if (endswith(filename.c_str(), ".pgm")) {
	/// load PGM file
      
	std::ifstream in(filename.c_str(), std::ios::binary);
	PGMHeader<Scalar> hdr(in);
	width  = hdr.width;
	height = hdr.height;
	numChannels = 1;
      
	d = hdr.readScalarVector(in);
      
	return;
      
      } else {
	throw std::runtime_error("File suffix not recognized trying to load "+filename);
      }
    
    }

    /** 
     * destroys this object
     */ 
    virtual ~Image() {
    }

    /** 
     * creates an image as a deep(!) copy of the image within. 
     * 
     * @param peer 
     */

    Image(const Image& peer) {
      operator=(peer);
    }

    /** 
     * makes this the deep(!) copy of peer. 
     * 
     * @param peer 
     * 
     * @return a reference to this
     */

    Image& operator=(const Image& peer) {
      width = peer.width;
      height = peer.height;
      numChannels = peer.numChannels;
      d = peer.d;
      return *this;	
    }

    /** 
     * @return the number of channels of the image
     */

    inline int getNumChannels() const {
      return numChannels;
    }

    /** 
     * @return the height of the image
     */  

    inline int getHeight() const {
      return height;
    }

    /** 
     * @return the width of the image
     */

    inline int getWidth() const {
      return width;
    }

    /** 
     * asserts width, height and numChannels match the respective entries of the
     * argument image
     * 
     * @param peer 
     */

    inline void assertDimensionsMatch (const Image& peer) const {
      assert(width == peer.width);
      assert(height == peer.height);
      assert(numChannels == peer.numChannels);    
    }

    /** 
     * tests whether peer's width, height and number of channels match this image's.
     * 
     * @param peer 
     */

    inline bool dimensionsMatch (const Image& peer) const {
      return (peer.width == width &&
	      peer.height == height &&
	      peer.numChannels == numChannels);
    }

    /** 
     * @return the data vector inside as a const reference
     */

    inline const std::vector<Scalar>& getDataVector() const {
      return d;
    }

    /** 
     * @return the data vector inside as a writable reference
     */

    inline std::vector<Scalar>& getDataVector() {
      return d;
    }

    /** 
     * @return the data vector inside as a pointer to Scalars
     */

    inline Scalar* getDataArrayAsScalars() {
      return &(d[0]);
    }

 
    /** 
     * direct access to the data vector within
     * 
     * @param i index
     * 
     * @return the i-th Scalar in d (basically d[i]).
     */

    inline Scalar& operator[](size_t i) {
      assert(i >= 0);
      assert(i < (size_t)width*height*numChannels);
      return d[i];
    }
  
    /** 
     * direct const access to the data vector within
     * 
     * @param i index
     * 
     * @return the i-th Scalar in d (basically d[i]).
     */

    inline const Scalar& operator[](size_t i) const {
      assert(i >= 0);
      assert(i < (size_t)width*height*numChannels);
      return d[i];
    }

    /** 
     * provides access to the image data
     *
     * @param x
     * @param y
     * @param c
     * 
     * @return an assignable Scalar of the x-th pixel in the y-th row in the c-th
     *channel of the image
     */

    inline Scalar& operator()(int x, int y, int c) {
      assert(x >= 0); assert (x < width);
      assert(y >= 0); assert (y < height);
      assert(c >= 0); assert (c < numChannels);
      return (d[c+numChannels*(x+width*y)]);
    }

    /** 
     * provides access to the image data
     *
     * @param x
     * @param y
     * @param c
     * 
     * @return an const Scalar reference of the x-th pixel in the y-th row in the c-th
     *channel of the image
     */

    inline const Scalar& operator()(int x, int y, int c) const {
      assert(x >= 0); assert (x < width);
      assert(y >= 0); assert (y < height);
      assert(c >= 0); assert (c < numChannels);
      return (d[c+numChannels*(x+width*y)]);
    }

    /**
     * saves the image, according to the file extension. pfm implies a
     * floating point file, ppm a 16 bit ppm file (scaling the output so
     * that a floating point value of 1.0 maps to 65535), pgm a 16 bit
     * ppm file with the same scaling.
     */

    void save(const std::string &filename) const {

      if (endswith(filename.c_str(), ".pfm")) {
	/// save PFM file
	
	//    assert (numChannels == 3);
	PFMHeader<Scalar> hdr(width, height, numChannels);
	
	std::ofstream out(filename.c_str(), std::ios_base::binary);
	hdr.write(out);
	hdr.writeScalarVector(out,d);
	return;
	
      } else if (endswith(filename.c_str(), ".ppm")) {
	/// save PPM file
	
	assert (numChannels == 3);
	PPMHeader<Scalar> hdr(width, height, 65535);

	std::ofstream out(filename.c_str(),std::ios_base::binary);
	hdr.write(out);
	hdr.writeScalarVector(out,d);
	return;
	
      } else if (endswith(filename.c_str(), ".pgm")) {
	/// save PGM file
	
	assert (numChannels == 1);
	PGMHeader<Scalar> hdr(width, height, 65535);

	std::ofstream out(filename.c_str(), std::ios_base::binary);
	hdr.write(out);
	hdr.writeScalarVector(out,d);
	return;
      } else {
	throw std::runtime_error("Couldn't save the image file: cannot determine the correct file extension from file name "+filename);
      }
      
    }
    
    /**
     * saves the image to an 8 bit or 16 bit pgm/ppm file. 1-channel
     * images are saved to pgm, 3-channel images are saved to ppm.
     * 
     * @param out stream to save in
     * @param maxval If 255 or less, saves to an 8 bit file, otherwise, saves to a 16 bit file.
     */

    void savepnm(std::ostream& out, int maxval = 255) const {
      if (numChannels == 1) {
	/// save PGM file
	
	assert (numChannels == 1);
	PGMHeader <Scalar> hdr(width, height, maxval);
	
	hdr.write(out);
	hdr.writeScalarVector(out,d);
	return;
	
      } else if (numChannels == 3) {
	/// save PPM file
	
	assert (numChannels == 3);
	PPMHeader <Scalar> hdr(width, height, maxval);
	
	hdr.write(out);
	hdr.writeScalarVector(out,d);
	return;
	
      } 
    }

    /** 
     * assuming all image intensities to be between 0 and 1, and the image to be
     * linearized, converts the color values to the non-linear sRGB function (but
     * still maps 0 to 0 and 1 to 1)
     * 
     */
    void sRGBRamp() {
    	/*
    	 * TODO insert code for linear to sRGB conversion here
    	 */
    }

    /** 
     * assuming all image intensities to be between 0 and 1, and the image to be
     * in sRBG space, converts the color values to linear values (but
     * still maps 0 to 0 and 1 to 1)
     * 
     */
    void sRGBRampInv() {
    	/*
    	 * TODO insert code for sRGB to linear conversion here
    	 */
    }

  
  protected:
    int width;			/**< width of the image */
    int height;			/**< height of the image */
    int numChannels;		/**< number of channels of the image */
    std::vector<Scalar> d;     	/**< the data inside */

  };

}
#endif
