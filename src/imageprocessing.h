#ifndef IMAGEPROCESSING_H_   /* Include guard */
#define IMAGEPROCESSING_H_

#include <stdlib.h>


#define FOUR                      (4) 
#define ALIGN_TO_FOUR(VAL)        (((VAL) + FOUR - 1) & ~(FOUR - 1))

#define CLEAR(x)                  memset(&(x), 0, sizeof(x))

#define R_DIFF(VV)                ((VV - 128) + (103*(VV - 128) >> 8))
#define G_DIFF(UU, VV)            (-((88*(UU - 128)) >> 8) - (((VV - 128)*183) >> 8 ))
#define B_DIFF(UU)                ((UU - 128) + ((198*(UU - 128)) >> 8))

#define Y_PLUS_RDIFF(YY, VV)      ((YY - 16) + R_DIFF(VV))
#define Y_PLUS_GDIFF(YY, UU, VV)  ((YY - 16) + G_DIFF(UU, VV))
#define Y_PLUS_BDIFF(YY, UU)      ((YY - 16) + B_DIFF(UU))

#define EVEN_ZERO_ODD_ONE(XX)     (((XX) & (0x01)))

/*if X > 255, X = 255; if X< 0, X = 0*/
#define CLIP1(XX)                 ((unsigned char)( (XX & ~255) ? (~XX >> 15) : XX ))


enum direction {
        VERTICAL,
        HORIZONTAL,
        VERTICAL_AND_HORIZONTAL,
};

typedef struct crop_w {
    int start_x;
    int start_y;
    int end_x;
    int end_y;
} crop_window;

typedef struct tagRGBQUAD {
  unsigned char rgbBlue;
  unsigned char rgbGreen;
  unsigned char rgbRed;
  unsigned char rgbReserved;
} RGBQUAD;

int BMPwriter(unsigned char *pRGB, int bitNum, int width, int height, char* output_filestring);
int GrayScaleWriter(unsigned char *pGrayscale, int width, int height, char* output_filestring);
int YUYV2RGB24(unsigned char *pYUYV, int width, int height, unsigned char *pRGB24);
int RGB24toGrayscale(unsigned char *inputRGB24, int width, int height, unsigned char *outputGrayscale);
int cropRGB24(unsigned char *inputRGB24, int width, int height, int startX, int startY, int endX, int endY, unsigned char* outputRGB24);
int makeZeroPaddedImage(unsigned char *inputGrayscale, int inputWidth, int inputHeight, int padWidth, unsigned char *outputGrayscale, enum direction ptype);
int convolve2D(double* kernel, int kernelSize, unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale);
int convolve2Dwith1Dkernel(double* kernel, int kernelSize, unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, enum direction dir);
int UniformBlur(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale);
void getGaussianKernel1D(double* kernel, double sigma, int kernelSize);
void getGaussianKernel2D(double* kernel, double sigma, int kernelSize);
int GaussianBlur2DKernel(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, double sigma);
int GaussianBlur(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, double sigma);
int DifferentialEdgeDetector(unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale, double sigma, double threshold, double cutoff_threshold);
int CornerDetector(unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale, double sigma, double sigma_w, double k, double threshold);

#endif
