#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#include "imageprocessing.h"


int BMPwriter(unsigned char *pRGB, int bitNum, int width, int height, char* output_filestring)
{
    FILE *fd_output; 
    int fileSize;
    unsigned char *pMovRGB;
    int i;
    int widthStep;

    unsigned char header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        54, 0, 0, 0, // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        24, 0,       // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution 
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    widthStep = ALIGN_TO_FOUR(width*bitNum/8);

    fileSize = ALIGN_TO_FOUR(widthStep*height) + sizeof(header);

    memcpy(&header[2], &fileSize, sizeof(int));
    memcpy(&header[18], &width, sizeof(int));
    memcpy(&header[22], &height, sizeof(int));
    memcpy(&header[28], &bitNum, sizeof(short)); 


    fprintf(stdout, "writing to file...");

    fd_output = fopen(output_filestring, "wb");

    fwrite(&header[0], 1, sizeof(header), fd_output);  

    pMovRGB  = pRGB + (height - 1)*widthStep; 

    for(i = 0; i < height; i++){
        fwrite(pMovRGB, 1, widthStep, fd_output);
        pMovRGB -= widthStep;
    }

    fflush(fd_output); 
    fclose(fd_output);
    fprintf(stdout, "done\n"); 

    return 0; 
}

int GrayScaleWriter(unsigned char *pGrayscale, int width, int height, char* output_filestring)
{
    FILE *fd_output; 
    int fileSize;
    const int NUMBER_OF_COLORS = 256;
    const int COLOR_PALETTE_SIZE = NUMBER_OF_COLORS * sizeof(RGBQUAD);
    RGBQUAD quad[NUMBER_OF_COLORS];
    int rgb_offset = 54 + COLOR_PALETTE_SIZE;
    unsigned char *pMovGrayscale;
    int i, widthStep;

    unsigned char header[54] = {
        0x42,        // identity : B
        0x4d,        // identity : M
        0, 0, 0, 0,  // file size
        0, 0,        // reserved1
        0, 0,        // reserved2
        0, 0, 0, 0,  // RGB data offset
        40, 0, 0, 0, // struct BITMAPINFOHEADER size
        0, 0, 0, 0,  // bmp width
        0, 0, 0, 0,  // bmp height
        1, 0,        // planes
        8, 0,        // bit per pixel
        0, 0, 0, 0,  // compression
        0, 0, 0, 0,  // data size
        0, 0, 0, 0,  // h resolution
        0, 0, 0, 0,  // v resolution 
        0, 0, 0, 0,  // used colors
        0, 0, 0, 0   // important colors
    };

    // Create the color palette for a grayscale image.
    // This sets each of the colors from 0 to 255 to a
    // shade of gray.
    for (i = 0; i < NUMBER_OF_COLORS; i++)
    {
        quad[i].rgbBlue = i;
        quad[i].rgbGreen = i;
        quad[i].rgbRed = i;
        quad[i].rgbReserved = 0;
    }

    widthStep = ALIGN_TO_FOUR(width);

    fileSize = ALIGN_TO_FOUR(widthStep)*height + sizeof(header) + COLOR_PALETTE_SIZE;

    memcpy(&header[2], &fileSize, sizeof(int));
    memcpy(&header[10], &rgb_offset, sizeof(int));
    memcpy(&header[18], &width, sizeof(int));
    memcpy(&header[22], &height, sizeof(int));


    fprintf(stdout, "writing to file...");

    fd_output = fopen(output_filestring, "wb");

    fwrite(&header[0], 1, sizeof(header), fd_output);
    fwrite(quad, 1, COLOR_PALETTE_SIZE, fd_output);

    pMovGrayscale  = pGrayscale + (height - 1)*widthStep; 

    for(i = 0; i < height; i++){
        fwrite(pMovGrayscale, 1, widthStep, fd_output);
        pMovGrayscale -= widthStep;
    }

    fflush(fd_output); 
    fclose(fd_output);
    fprintf(stdout, "done\n"); 

    return 0;
}

/*
*  Function: YUYV2RGB24
*  --------------------
*  Converts a YUYV bitmap to a true color (i.e. 24 bit depth) RGB bitmap
*
*  pYUYV:  A pointer to the pixels of a YUYV bitmap in memory.
*  width:  The width of the image.
*  height: The height of the image.
*  pRGB24: A pointer to memory allocated to hold the pixels of the RGB bitmap.
*
*  Bitmaps are stored as continuous runs of pixels in memory. In order to access
*  a particular (x, y) pixel, you need to address it by some variation
*  of the following formula:
*     start_address + y*(number fo pixels in width) + x
*
*  The variation in the formula come down to memory access
*  considerations (more on that later).
*
*  Below is a diagram of a small 4 x 4 square patch of a YUYV bitmap:
*
*  YUYVYUYV
*  YUYVYUYV
*
*  i.e. Each pixel contains two values, either YU or YV.
*  From https://en.wikipedia.org/wiki/YUV, we are told that Y is the
*  luminance channel and indicates brightniss of the pixel and U and V
*  represent the chrominance. Each value takes up a byte of space,
*  and so each pixel takes up two bytes of space.
*
*  There is some formula for YUV --> RGB colorspace conversion being
*  used here. (todo: (Xavier) Add more details).
*
*  RGB24 bitmaps are continuous runs of pixels stored as BGR, i.e. blue,
*  green and red values. Each value takes up a byte of space, and so each
*  pixel takes up three bytes of space, i.e. 24 bits.
*/
int YUYV2RGB24(unsigned char *pYUYV, int width, int height, unsigned char *pRGB24)
{
    unsigned int i, j;

    unsigned char *pMovY, *pMovU, *pMovV;
    unsigned char *pMovRGB;

    unsigned int pitch;
    unsigned int pitchRGB;

    // Number of bytes in one row of pixels in the YUYV bitmap.
    pitch = 2*width;

    // Number of bytes in one row of pixels in the RGB bitmap.
    // However for memory access reasons, i.e. needing to be able
    // to do aligned reads, this needs to be a multiple of 4.
    // In the YUYV bitmap this is already the case.
    // For the RGB bitmap, the number of bytes hoding each row
    // of pixels of the image may thus be slightly larger, with
    // some wasteage on the ends.
    pitchRGB = ALIGN_TO_FOUR(3*width);


    for(j = 0; j< height; j++){
        pMovY = pYUYV + j*pitch;
        pMovU = pMovY + 1; 
        pMovV = pMovY + 3; 

        pMovRGB = pRGB24 + pitchRGB*j;

        for(i = 0; i< width; i++){
            int R, G, B;  
            int Y, U, V;

            // YUV --> RGB colorspace conversion.
            Y = pMovY[0]; U = *pMovU; V = *pMovV;   
            R = Y_PLUS_RDIFF(Y, V);
            G = Y_PLUS_GDIFF(Y, U, V);
            B = Y_PLUS_BDIFF(Y, U);

            // Clipping the value to the 0 - 255 range.
            *pMovRGB = CLIP1(B);
            *(pMovRGB + 1) = CLIP1(G);     
            *(pMovRGB + 2) = CLIP1(R);   

            // Move the pointer to the next Y value to use.
            pMovY += 2;

            // Move the U and V value pointers to the next U and V values to use.
            pMovU += 4*EVEN_ZERO_ODD_ONE(i);
            pMovV += 4*EVEN_ZERO_ODD_ONE(i);

            // Move the pointer to the start of the next BGR pixel in the RGB bitmap.
            pMovRGB += 3;
        }
    }

    return 0;
}

/*
*  Function: RGB24toGrayscale
*  --------------------------
*
*  inputRGB24       Pointer to the bytes in memory holding the pixels of the RGB24 bitmap.
*  width            Width in pixels of the input RGB24 bitmap.
*  height           Height in pixels of the input RGB24 bitmap.
*  outputGrayscale  Pointer to the bytes in memory which will hold the pixels of the 8-bit
*                   grayscale bitmap.
*
*  This function calculates the luminance Y of each pixel of the input RGB24 bitmap, and
*  clamps its value to the 0 - 255 range. It then assigns this value to the corresponding
*  pixel of the output grayscale bitmap.
*/
int RGB24toGrayscale(unsigned char *inputRGB24, int width, int height, unsigned char *outputGrayscale)
{
    unsigned int i, j;
    unsigned int pitchInputRGB, pitchOutputGrayscale;
    unsigned char* pMovInputRGB;
    unsigned char* pMovOutputGrayscale;

    pitchInputRGB = ALIGN_TO_FOUR(3*width);
    pitchOutputGrayscale = ALIGN_TO_FOUR(width);
    pMovOutputGrayscale = outputGrayscale;

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            int R, G, B, Y;
            pMovInputRGB = inputRGB24 + j*pitchInputRGB + i*3;
            pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;

            B = *pMovInputRGB;
            G = *(pMovInputRGB + 1);
            R = *(pMovInputRGB + 2);

            // The luminance Y is calculated from the R, G, B values.
            // These values were taken from this Stackoverflow here:
            // https://stackoverflow.com/questions/687261/converting-rgb-to-grayscale-intensity
            // When I applied the gamma crrection, the entire grayscale
            // image came out white. And when I didn't apply the gamma
            // correction, I got a grayscale looking image.
            // Additionally, check out this reference here:
            // http://cadik.posvete.cz/color_to_gray_evaluation/
            Y = 0.2126 * (double)R  + 0.7152 * (double)G + 0.722 * (double)B;
            if (Y > 255) Y = 255;
            else if (Y < 0) Y = 0;

            *pMovOutputGrayscale = (unsigned char)Y;
        }
    }

    return 0;
}

/*
*  Function: cropRGB24
*  -------------------
*
*  inputRGB24   Pointer to the input RGB24 bitmap bytes in memory.
*  width        The width in pixels of the input bitmap.
*  height       The height in pixels of the input bitmap.
*  startX       The x-coordinate of the top left corner of the crop window.
*  startY       The y-coordinate of the top left corner of the crop window.
*  endX         The x-coordinate of the bottom right corner of the crop window.
*  endY         The y-coordinate of the bottom right corner of the crop window.
*  outputRGB24  Pointer to the start of the memory region allocated for holding
*               the bytes of the output RGB24 bitmap.
*/
int cropRGB24(unsigned char *inputRGB24, int width, int height, int startX, int startY, int endX, int endY, unsigned char* outputRGB24)
{
    unsigned int i, j;
    unsigned int pitchInputRGB, pitchOutputRGB;
    unsigned char* pMovInputRGB;
    unsigned char* pMovOutputRGB;

    pitchInputRGB = ALIGN_TO_FOUR(3*width);
    pitchOutputRGB = ALIGN_TO_FOUR(3*(endX - startX));

    // Iterate over all the pixels from the input RGB24 bitmap that
    // are within the crop window, and simply copy the BGR values to
    // the allocated memory.
    for(j = startY; j < endY; j++) {
        for(i = startX; i < endX; i++) {
            // The start memory location of the (i, j)th pixel of the
            // input RGB24 bitmap.
            pMovInputRGB = inputRGB24 + j*pitchInputRGB + i*3;

            // The start memory location of the (i, j)th pixel of the
            // output RGB24 bitmap.
            pMovOutputRGB = outputRGB24 + (j - startY)*pitchOutputRGB + (i - startX)*3;

            // Assign the BGR values of the (i, j)th pixel of the input RGB bitmap
            // to the BGR values of the (i, j)th pixel of the output RGB bitmap.
            *pMovOutputRGB = *pMovInputRGB;
            *(pMovOutputRGB + 1) = *(pMovInputRGB + 1);
            *(pMovOutputRGB + 2) = *(pMovInputRGB + 2);
        }
    }

    return 0;

}

int makeZeroPaddedImage(double *inputGrayscale, int inputWidth, int inputHeight, int padWidth, double *outputGrayscale,
                        enum direction ptype)
{
    unsigned int i, j;
    unsigned int pitchInputGrayscale, pitchOutputGrayscale;
    double* pMovInputGrayscale;
    double* pMovOutputGrayscale;

    pMovInputGrayscale = inputGrayscale;
    pMovOutputGrayscale = outputGrayscale;

    if (ptype == VERTICAL_AND_HORIZONTAL) {
        pitchInputGrayscale = inputWidth;
        pitchOutputGrayscale = inputWidth + 2*padWidth;

        for(j=padWidth; j < (inputHeight + padWidth); j++) {
            for(i=padWidth; i < (inputWidth + padWidth); i++) {
                pMovInputGrayscale = inputGrayscale + (j - padWidth)*pitchInputGrayscale + (i - padWidth);
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = *pMovInputGrayscale;
            }
        }

        for(j=0; j < padWidth; j++) {
            for(i=0; i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }

        for(j=(padWidth + inputHeight); j < (inputHeight + 2*padWidth); j++) {
            for(i=0; i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }

        for(j=0; j < (2*padWidth + inputHeight); j++) {
            for(i=0; i < padWidth; i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }

        for(j=0; j < (2*padWidth + inputHeight); j++) {
            for(i=(padWidth + inputWidth); i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }
    } else if (ptype == VERTICAL) {
        pitchInputGrayscale = inputWidth;
        pitchOutputGrayscale = inputWidth;

        for(j=padWidth; j < (inputHeight + padWidth); j++) {
            for(i=0; i < inputWidth; i++) {
                pMovInputGrayscale = inputGrayscale + (j - padWidth)*pitchInputGrayscale + i;
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = *pMovInputGrayscale;
            }
        }

        for(j=0; j < padWidth; j++) {
            for(i=0; i < inputWidth; i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }

        for(j=(padWidth + inputHeight); j < (inputHeight + 2*padWidth); j++) {
            for(i=0; i < inputWidth; i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }
    } else if (ptype == HORIZONTAL) {
        pitchInputGrayscale = inputWidth;
        pitchOutputGrayscale = inputWidth + 2*padWidth;

        for(j=0; j < inputHeight; j++) {
            for(i=padWidth; i < (inputWidth + padWidth); i++) {
                pMovInputGrayscale = inputGrayscale + j*pitchInputGrayscale + (i - padWidth);
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = *pMovInputGrayscale;
            }
        }

        for(j=0; j < inputHeight; j++) {
            for(i=0; i < padWidth; i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }

        for(j=0; j < inputHeight; j++) {
            for(i=(padWidth + inputWidth); i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = 0.0;
            }
        }
    }

    return 0;
}

int convolve2D(double* kernel, int kernelSize, unsigned char* inputGrayscale, int width, int height, double* outputGrayscale)
{
    unsigned int padWidth;
    unsigned int i, j, k, l;
    unsigned int pitchPaddedGrayscale, pitchOutputGrayscale;
    double result;

    double* double_input_grayscale;
    double* paddedGrayscale;
    double* pMovPaddedGrayscale;
    double* pMovOutputGrayscale;
    double* pMovKernel;

    padWidth = (kernelSize - 1) / 2;

    double_input_grayscale = (double*)malloc(width*height*sizeof(double));
    paddedGrayscale = (double*)malloc(((width + 2*padWidth)*(height + 2*padWidth)) * sizeof(double));

    convertUcharToDoubleGrayscale(inputGrayscale, width, height, double_input_grayscale);
    makeZeroPaddedImage(double_input_grayscale, width, height, padWidth, paddedGrayscale, VERTICAL_AND_HORIZONTAL);

    pitchPaddedGrayscale = width + 2*padWidth;
    pitchOutputGrayscale = width;

    for(j=padWidth; j < (padWidth + height); j++) {
        for(i=padWidth; i < (padWidth + width); i++) {
            pMovOutputGrayscale = outputGrayscale + (j - padWidth)*pitchOutputGrayscale + (i - padWidth);
            pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + i;
            result = 0.0;
            for(k=0; k < kernelSize; k++) {
                for(l=0; l < kernelSize; l++) {
                    pMovKernel = kernel + (kernelSize - k - 1)*kernelSize + (kernelSize - l - 1);
                    pMovPaddedGrayscale = paddedGrayscale + (j - padWidth + k)*pitchPaddedGrayscale + (i - padWidth + l);
                    result = result + (*pMovKernel) * (*pMovPaddedGrayscale);
                }
            }

            if (result > 255) result = 255;
            else if (result < 0) result = 0;

            *pMovOutputGrayscale = result;
        }
    }

    free(paddedGrayscale);
    free(double_input_grayscale);

    return 0;
}

int convolve2Dwith1Dkernel(double* kernel, int kernelSize, double* inputGrayscale, int width, int height, double* outputGrayscale, enum direction dir)
{
    unsigned int padWidth;
    unsigned int i, j, k;
    unsigned int pitchPaddedGrayscale, pitchOutputGrayscale;
    double result;

    double* paddedGrayscale;
    double* pMovPaddedGrayscale;
    double* pMovOutputGrayscale;
    double* pMovKernel;

    padWidth = (kernelSize - 1) / 2;

    if (dir == VERTICAL) {
        paddedGrayscale = (double*)malloc(width*(height + 2*padWidth)*sizeof(double));
        makeZeroPaddedImage(inputGrayscale, width, height, padWidth, paddedGrayscale, VERTICAL);

        pitchPaddedGrayscale = width;
        pitchOutputGrayscale = width;

        for(j=padWidth; j < (padWidth + height); j++) {
            for(i=0; i < width; i++) {
                pMovOutputGrayscale = outputGrayscale + (j - padWidth)*pitchOutputGrayscale + i;
                pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + i;
                result = 0.0;
                for(k=0; k < kernelSize; k++) {
                    pMovKernel = kernel + (kernelSize - k - 1);
                    pMovPaddedGrayscale = paddedGrayscale + (j - padWidth + k)*pitchPaddedGrayscale + i;
                    result = result + (*pMovKernel) * (*pMovPaddedGrayscale);
                }

                *pMovOutputGrayscale = result;
            }
        }

        free(paddedGrayscale);

        return 0;

    } else if (dir == HORIZONTAL) {
        paddedGrayscale = (double*)malloc((width + 2*padWidth)*height*sizeof(double));
        makeZeroPaddedImage(inputGrayscale, width, height, padWidth, paddedGrayscale, HORIZONTAL);

        pitchPaddedGrayscale = width + 2*padWidth;
        pitchOutputGrayscale = width;

        for(j=0; j < height; j++) {
            for(i=padWidth; i < (padWidth + width); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + (i - padWidth);
                pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + i;
                result = 0.0;
                for(k=0; k < kernelSize; k++) {
                    pMovKernel = kernel + (kernelSize - k - 1);
                    pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + (i - padWidth + k);
                    result = result + (*pMovKernel) * (*pMovPaddedGrayscale);
                }

                *pMovOutputGrayscale = result;
            }
        }

        free(paddedGrayscale);

        return 0;
    }

    return -1;
}

int UniformBlur(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale)
{
    double* intermediateGrayscale;

    double kernel[9] =  {
        1/9.0, 1/9.0, 1/9.0,
        1/9.0, 1/9.0, 1/9.0,
        1/9.0, 1/9.0, 1/9.0
    };

    intermediateGrayscale = (double *)malloc(width*height*sizeof(double));

    convolve2D(kernel, 3, inputGrayscale, width, height, intermediateGrayscale);
    convertDoubleToUcharGrayscale(intermediateGrayscale, width, height, outputGrayscale);

    free(intermediateGrayscale);

    return 0;
}

void getGaussianKernel1D(double* kernel, double sigma, int kernelSize)
{
    int i, max;
    double value, sum;

    max = (kernelSize - 1) / 2.0;
    sum = max;

    kernel[max] = 1/(sigma *sqrt(2 * M_PI));
    for(i=1; i <= max; i++) {
        value = exp(-i*i/(2 * sigma * sigma))/(sigma *sqrt(2 * M_PI));
        kernel[max - i] = value;
        kernel[max + i] = value;
        sum = sum + 2*value;
    }
}

void getDGausianKernel1D(double* kernel, double sigma, int kernelSize)
{
    int i, max;
    double value, sum;

    max = (kernelSize - 1) / 2.0;
    sum = 0.0;

    for(i=0; i <= 2 * max; i++) {
        value = (-(i - max)/(sigma * sigma)) * exp(-(i - max)*(i - max)/(2 * sigma * sigma))/(sigma *sqrt(2 * M_PI));
        kernel[i] = value;
        sum = sum + kernel[i];
    }
}

void getD2GausianKernel1D(double* kernel, double sigma, int kernelSize)
{
    int i, max;
    double value, sum;

    max = (kernelSize - 1) / 2.0;
    sum = 0.0;

    for(i=0; i <= 2 * max; i++) {
        value = (-1.0/(sigma * sigma) + ((i - max) * (i - max)/(sigma * sigma * sigma * sigma))) * exp(-(i - max)*(i - max)/(2 * sigma * sigma))/(sigma *sqrt(2 * M_PI));
        kernel[i] = value;
        sum = sum + kernel[i];
    }
}

void getGaussianKernel2D(double* kernel, double sigma, int kernelSize)
{
    int i, j, max;
    double value, sum;
    double* pKernelMov;

    max = (kernelSize - 1) / 2.0;

    pKernelMov = kernel + max*kernelSize + max;

    sum = 0.0;
    for(j=0; j < kernelSize; j++) {
        for(i=0; i < kernelSize; i++) {
            pKernelMov = kernel + j*kernelSize + i;
            value = (double)exp(-((i - max)*(i - max) + ((j - max)*(j - max)))/(2 * sigma * sigma))/(sigma *sqrt(2 * M_PI));
            *pKernelMov = value;
            sum = sum + value;
        }
    }

    for(i=0; i < (kernelSize * kernelSize); i++) {
        kernel[i] = kernel[i] / sum;
    }
}

int GaussianDerivativeX(double* input_grayscale, int width, int height, double* output_grayscale, double sigma)
{
    double* kernel_x;
    double* kernel_y;
    int kernel_size;
    double* temp_grayscale_v;

    kernel_size = 2 * ((int) (5*sigma)) + 1;

    temp_grayscale_v = (double*)malloc(width * height * sizeof(double));
    kernel_x = (double *)malloc(kernel_size * sizeof(double));
    kernel_y = (double *)malloc(kernel_size * sizeof(double));

    getDGausianKernel1D(kernel_x, sigma, kernel_size);
    getGaussianKernel1D(kernel_y, sigma, kernel_size);

    convolve2Dwith1Dkernel(kernel_y, kernel_size, input_grayscale, width, height, temp_grayscale_v, VERTICAL);
    convolve2Dwith1Dkernel(kernel_x, kernel_size, temp_grayscale_v, width, height, output_grayscale, HORIZONTAL);

    free(temp_grayscale_v);
    free(kernel_x);
    free(kernel_y);

    return 0;
}

int GaussianDerivativeY(double* input_grayscale, int width, int height, double* output_grayscale, double sigma)
{
    double* kernel_x;
    double* kernel_y;
    int kernel_size;
    double* temp_grayscale_v;

    kernel_size = 2 * ((int) (5*sigma)) + 1;

    temp_grayscale_v = (double*)malloc(width * height * sizeof(double));
    kernel_x = (double *)malloc(kernel_size * sizeof(double));
    kernel_y = (double *)malloc(kernel_size * sizeof(double));

    getDGausianKernel1D(kernel_y, sigma, kernel_size);
    getGaussianKernel1D(kernel_x, sigma, kernel_size);

    convolve2Dwith1Dkernel(kernel_y, kernel_size, input_grayscale, width, height, temp_grayscale_v, VERTICAL);
    convolve2Dwith1Dkernel(kernel_x, kernel_size, temp_grayscale_v, width, height, output_grayscale, HORIZONTAL);

    free(temp_grayscale_v);
    free(kernel_x);
    free(kernel_y);

    return 0;
}

int GaussianDerivativeXX(double* input_grayscale, int width, int height, double* output_grayscale, double sigma)
{
    double* kernel_x;
    double* kernel_y;
    int kernel_size;
    double* temp_grayscale_v;

    kernel_size = 2 * ((int) (5*sigma)) + 1;

    temp_grayscale_v = (double*)malloc(width * height * sizeof(double));
    kernel_x = (double *)malloc(kernel_size * sizeof(double));
    kernel_y = (double *)malloc(kernel_size * sizeof(double));

    getD2GausianKernel1D(kernel_x, sigma, kernel_size);
    getGaussianKernel1D(kernel_y, sigma, kernel_size);

    convolve2Dwith1Dkernel(kernel_y, kernel_size, input_grayscale, width, height, temp_grayscale_v, VERTICAL);
    convolve2Dwith1Dkernel(kernel_x, kernel_size, temp_grayscale_v, width, height, output_grayscale, HORIZONTAL);

    free(temp_grayscale_v);
    free(kernel_x);
    free(kernel_y);

    return 0;
}

int GaussianDerivativeYY(double* input_grayscale, int width, int height, double* output_grayscale, double sigma)
{
    double* kernel_x;
    double* kernel_y;
    int kernel_size;
    double* temp_grayscale_v;

    kernel_size = 2 * ((int) (5*sigma)) + 1;

    temp_grayscale_v = (double*)malloc(width * height * sizeof(double));
    kernel_x = (double *)malloc(kernel_size * sizeof(double));
    kernel_y = (double *)malloc(kernel_size * sizeof(double));

    getD2GausianKernel1D(kernel_y, sigma, kernel_size);
    getGaussianKernel1D(kernel_x, sigma, kernel_size);

    convolve2Dwith1Dkernel(kernel_y, kernel_size, input_grayscale, width, height, temp_grayscale_v, VERTICAL);
    convolve2Dwith1Dkernel(kernel_x, kernel_size, temp_grayscale_v, width, height, output_grayscale, HORIZONTAL);

    free(temp_grayscale_v);
    free(kernel_x);
    free(kernel_y);

    return 0;
}

int GaussianDerivativeXY(double* input_grayscale, int width, int height, double* output_grayscale, double sigma)
{
    double* kernel_x;
    double* kernel_y;
    int kernel_size;
    double* temp_grayscale_v;

    kernel_size = 2 * ((int) (5*sigma)) + 1;

    temp_grayscale_v = (double*)malloc(width * height * sizeof(double));
    kernel_x = (double *)malloc(kernel_size * sizeof(double));
    kernel_y = (double *)malloc(kernel_size * sizeof(double));

    getGaussianKernel1D(kernel_x, sigma, kernel_size);
    getGaussianKernel1D(kernel_y, sigma, kernel_size);

    convolve2Dwith1Dkernel(kernel_x, kernel_size, input_grayscale, width, height, temp_grayscale_v, VERTICAL);
    convolve2Dwith1Dkernel(kernel_y, kernel_size, temp_grayscale_v, width, height, output_grayscale, HORIZONTAL);

    free(temp_grayscale_v);
    free(kernel_x);
    free(kernel_y);

    return 0;
}

int GaussianBlur2DKernel(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, double sigma)
{
    double* kernel;
    double* intermediateGrayscale;
    int kernelSize;

    kernelSize = 2 * ((int) (3*sigma)) + 1;

    kernel = (double *)malloc(kernelSize * kernelSize * sizeof(double));
    intermediateGrayscale = (double *)malloc(width*height*sizeof(double));

    getGaussianKernel2D(kernel, sigma, kernelSize);

    convolve2D(kernel, kernelSize, inputGrayscale, width, height, intermediateGrayscale);
    convertDoubleToUcharGrayscale(intermediateGrayscale, width, height, outputGrayscale);

    free(kernel);
    free(intermediateGrayscale);

    return 0;
}


int zero_crossing_patch(double* pixel_values, int length)
{
    int num_zero_or_positive = 0;
    int num_zero_or_negative = 0;

    for(int i=0; i < length; i++) {
        if (pixel_values[i] <= 0)
            num_zero_or_negative++;
        if (0 <= pixel_values[i])
            num_zero_or_positive++;
    }

    if (num_zero_or_positive > 0 && num_zero_or_negative > 0)
        return 1;

    return 0;
}


int DifferentialEdgeDetector(unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale, double sigma, double threshold, double cutoff_threshold)
{
    unsigned int i, j, pitch, nedges;
    double* double_input_grayscale;
    double* input_grayscale_DX;
    double* input_grayscale_DXX;
    double* input_grayscale_DY;
    double* input_grayscale_DYY;
    double* input_grayscale_DXY;
    double* p_DX;
    double* p_DXX;
    double* p_DY;
    double* p_DYY;
    double* p_DXY;
    double* gradient_norm;
    double** hedges;
    unsigned char* p_output;
    double* p_gradient_norm;
    double* p_second_order;
    double norm_squared_gradient, squared_threshold, squared_cutoff_threshold;
    double* second_order;

    double_input_grayscale = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DX = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DXX = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DY = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DYY = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DXY = (double*)malloc(width * height * sizeof(double));
    gradient_norm = (double*)malloc(width * height * sizeof(double));
    second_order = (double*)malloc(width * height * sizeof(double));
    hedges = (double**)malloc(width * height * sizeof(double**));

    convertUcharToDoubleGrayscale(input_grayscale, width, height, double_input_grayscale);
    GaussianDerivativeX(double_input_grayscale, width, height, input_grayscale_DX, sigma);
    GaussianDerivativeXX(double_input_grayscale, width, height, input_grayscale_DXX, sigma);
    GaussianDerivativeY(double_input_grayscale, width, height, input_grayscale_DY, sigma);
    GaussianDerivativeYY(double_input_grayscale, width, height, input_grayscale_DYY, sigma);
    GaussianDerivativeXY(double_input_grayscale, width, height, input_grayscale_DXY, sigma);

    squared_threshold = threshold * threshold;
    squared_cutoff_threshold = cutoff_threshold * cutoff_threshold;

    pitch = ALIGN_TO_FOUR(width);

    for(j=0; j < height; j++) {
        for (i=0; i < width; i++) {
            p_DX = input_grayscale_DX + width*j + i;
            p_DXX = input_grayscale_DXX + width*j + i;
            p_DY = input_grayscale_DY + width*j + i;
            p_DYY = input_grayscale_DYY + width*j + i;
            p_DXY = input_grayscale_DXY + width*j + i;
            p_second_order = second_order + width * j + i;

            *p_second_order = ((*p_DX) * (*p_DX) * (*p_DXX)) + (2 * (*p_DX) * (*p_DY) * (*p_DXY)) + ((*p_DY) * (*p_DY) * (*p_DYY));
        }
    }

    for(j=1; j < (height - 1); j++) {
        for(i=1; i < (width - 1); i++) {
            p_DX = input_grayscale_DX + width*j + i;
            p_DY = input_grayscale_DY + width*j + i;
            p_gradient_norm = gradient_norm + width * j + i;
            p_second_order = second_order + width*j + i;

            norm_squared_gradient = (*p_DX) * (*p_DX) + (*p_DY) * (*p_DY);

            double nbr_seo[8];
            nbr_seo[0] = *(p_second_order + (j + 1)*width + i);
            nbr_seo[1] = *(p_second_order + (j - 1)*width + i);
            nbr_seo[2] = *(p_second_order + j*width + (i + 1));
            nbr_seo[3] = *(p_second_order + j*width + (i - 1));
            nbr_seo[4] = *(p_second_order + (j + 1)*width + (i + 1));
            nbr_seo[5] = *(p_second_order + (j - 1)*width + (i - 1));
            nbr_seo[6] = *(p_second_order + (j + 1)*width + (i - 1));
            nbr_seo[7] = *(p_second_order + (j - 1)*width + (i + 1));

            if ((norm_squared_gradient >= squared_threshold) && (zero_crossing_patch(nbr_seo, 8))) {
                *p_gradient_norm = norm_squared_gradient;
            }
            else {
                *p_gradient_norm = 0.0;
            }

        }
    }

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_gradient_norm = gradient_norm + width * j + i;

            if (*p_gradient_norm >= squared_threshold) {
                hedges[0] = p_gradient_norm;
                nedges = 1;

                do {
                    nedges--;
                    double* nbrs[8];
                    nbrs[0] = hedges[nedges] + width;
                    nbrs[1] = hedges[nedges] - width;
                    nbrs[2] = hedges[nedges] - 1;
                    nbrs[3] = hedges[nedges] + 1;
                    nbrs[4] = hedges[nedges] + width + 1;
                    nbrs[5] = hedges[nedges] + width - 1;
                    nbrs[6] = hedges[nedges] - width + 1;
                    nbrs[7] = hedges[nedges] - width - 1;
                    for (int k=0; k < 8; k++) {
                        if ((*(nbrs[k]) >= squared_cutoff_threshold) && (*(nbrs[k]) < squared_threshold)) {
                            *(nbrs[k]) = squared_threshold;
                            nedges++;
                            hedges[nedges] = nbrs[k];
                        }
                    }

                } while(nedges > 0);
            }
        }
    }

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_gradient_norm = gradient_norm + width * j + i;
            p_output = output_grayscale + pitch * j + i;

            *p_output = 0;
            if(*p_gradient_norm >= squared_threshold) {
                *p_output = 255;
            }
        }
    }


    free(input_grayscale_DX);
    free(input_grayscale_DXX);
    free(input_grayscale_DY);
    free(input_grayscale_DYY);
    free(input_grayscale_DXY);
    free(double_input_grayscale);
    free(gradient_norm);
    free(second_order);
    free(hedges);

    return 0;
}


int CannyEdgeDetector(unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale, double sigma, double threshold, double cutoff_threshold)
{
    unsigned int i, j, pitch, nedges;
    double* double_input_grayscale;
    double* input_grayscale_DX;
    double* input_grayscale_DXX;
    double* input_grayscale_DY;
    double* input_grayscale_DYY;
    double* input_grayscale_DXY;
    double* p_DX;
    double* p_DY;
    double* gradient_norm;
    double** hedges;
    unsigned char* p_output;
    double* p_gradient_norm;
    double norm_squared_gradient, squared_threshold, squared_cutoff_threshold;
    double n_norm_squared_gradient, s_norm_squared_gradient, e_norm_squared_gradient, w_norm_squared_gradient, ne_norm_squared_gradient,
           nw_norm_squared_gradient, se_norm_squared_gradient, sw_norm_squared_gradient, dir;

    double_input_grayscale = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DX = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DXX = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DY = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DYY = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DXY = (double*)malloc(width * height * sizeof(double));
    gradient_norm = (double*)malloc(width * height * sizeof(double));
    hedges = (double**)malloc(width * height * sizeof(double**));

    convertUcharToDoubleGrayscale(input_grayscale, width, height, double_input_grayscale);
    GaussianDerivativeX(double_input_grayscale, width, height, input_grayscale_DX, sigma);
    GaussianDerivativeXX(double_input_grayscale, width, height, input_grayscale_DXX, sigma);
    GaussianDerivativeY(double_input_grayscale, width, height, input_grayscale_DY, sigma);
    GaussianDerivativeYY(double_input_grayscale, width, height, input_grayscale_DYY, sigma);
    GaussianDerivativeXY(double_input_grayscale, width, height, input_grayscale_DXY, sigma);

    squared_threshold = threshold * threshold;
    squared_cutoff_threshold = cutoff_threshold * cutoff_threshold;

    pitch = ALIGN_TO_FOUR(width);

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_DX = input_grayscale_DX + width*j + i;
            p_DY = input_grayscale_DY + width*j + i;
            p_gradient_norm = gradient_norm + width * j + i;

            norm_squared_gradient = (*p_DX) * (*p_DX) + (*p_DY) * (*p_DY);
            n_norm_squared_gradient = ((*(p_DX + width)) * (*(p_DX + width))) + ((*(p_DY + width)) * (*(p_DY + width)));
            s_norm_squared_gradient = ((*(p_DX - width)) * (*(p_DX - width))) + ((*(p_DY - width)) * (*(p_DY - width)));
            e_norm_squared_gradient = ((*(p_DX - 1)) * (*(p_DX - 1))) + ((*(p_DY - 1)) * (*(p_DY - 1)));
            w_norm_squared_gradient = ((*(p_DX + 1)) * (*(p_DX + 1))) + ((*(p_DY + 1)) * (*(p_DY + 1)));
            ne_norm_squared_gradient = ((*(p_DX + width + 1)) * (*(p_DX + width + 1))) + ((*(p_DY + width + 1)) * (*(p_DY + width + 1)));
            nw_norm_squared_gradient = ((*(p_DX + width - 1)) * (*(p_DX + width - 1))) + ((*(p_DY + width - 1)) * (*(p_DY + width - 1)));
            se_norm_squared_gradient = ((*(p_DX - width + 1)) * (*(p_DX - width + 1))) + ((*(p_DY - width + 1)) * (*(p_DY - width + 1)));
            sw_norm_squared_gradient = ((*(p_DX - width - 1)) * (*(p_DX - width - 1))) + ((*(p_DY - width - 1)) * (*(p_DY - width - 1)));

            dir = (fmod(atan2(*p_DY, *p_DX) + M_PI, M_PI) / M_PI) * 8;
            if (((dir <= 1 || dir > 7) && ((norm_squared_gradient > e_norm_squared_gradient) && (norm_squared_gradient > w_norm_squared_gradient))) ||
                ((dir > 1 && dir <= 3) && ((norm_squared_gradient > nw_norm_squared_gradient) && (norm_squared_gradient > se_norm_squared_gradient))) ||
                ((dir > 3 && dir <= 5) && ((norm_squared_gradient > n_norm_squared_gradient) && (norm_squared_gradient > s_norm_squared_gradient))) ||
                ((dir > 5 && dir <= 7) && ((norm_squared_gradient > ne_norm_squared_gradient) && (norm_squared_gradient > sw_norm_squared_gradient)))) {
                *p_gradient_norm = norm_squared_gradient;
            }
            else {
                *p_gradient_norm = 0.0;
            }

        }
    }

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_gradient_norm = gradient_norm + width * j + i;

            if (*p_gradient_norm >= squared_threshold) {
                hedges[0] = p_gradient_norm;
                nedges = 1;

                do {
                    nedges--;
                    double* nbrs[8];
                    nbrs[0] = hedges[nedges] + width;
                    nbrs[1] = hedges[nedges] - width;
                    nbrs[2] = hedges[nedges] - 1;
                    nbrs[3] = hedges[nedges] + 1;
                    nbrs[4] = hedges[nedges] + width + 1;
                    nbrs[5] = hedges[nedges] + width - 1;
                    nbrs[6] = hedges[nedges] - width + 1;
                    nbrs[7] = hedges[nedges] - width - 1;
                    for (int k=0; k < 8; k++) {
                        if ((*(nbrs[k]) >= squared_cutoff_threshold) && (*(nbrs[k]) < squared_threshold)) {
                            *(nbrs[k]) = squared_threshold;
                            nedges++;
                            hedges[nedges] = nbrs[k];
                        }
                    }

                } while(nedges > 0);
            }
        }
    }

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_gradient_norm = gradient_norm + width * j + i;
            p_output = output_grayscale + pitch * j + i;

            *p_output = 0;
            if(*p_gradient_norm >= squared_threshold) {
                *p_output = 255;
            }
        }
    }


    free(input_grayscale_DX);
    free(input_grayscale_DXX);
    free(input_grayscale_DY);
    free(input_grayscale_DYY);
    free(input_grayscale_DXY);
    free(double_input_grayscale);
    free(gradient_norm);
    free(hedges);

    return 0;
}

int CornerDetector(unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale, double sigma, double sigma_w, double k, double threshold)
{
    unsigned int i, j, pitch;
    double* double_input_grayscale;
    double* input_grayscale_DX_squared;
    double* input_grayscale_DY_squared;
    double* input_grayscale_DXDY;
    double* s_x2;
    double* s_y2;
    double* s_xy;
    double* p_DX_squared;
    double* p_DY_squared;
    double* p_DXDY;
    double* p_sx2;
    double* p_sy2;
    double* p_sxy;
    unsigned char* p_output;
    double harris_corner_response;
    double* harris_corner_response_image;
    double* p_harris_corner_response_image;

    harris_corner_response_image = (double*)malloc(width*height*sizeof(double));
    double_input_grayscale = (double*)malloc(width*height*sizeof(double));
    input_grayscale_DX_squared = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DY_squared = (double*)malloc(width * height * sizeof(double));
    input_grayscale_DXDY = (double*)malloc(width * height * sizeof(double));
    s_x2 = (double*)malloc(width * height * sizeof(double));
    s_y2 = (double*)malloc(width * height * sizeof(double));
    s_xy = (double*)malloc(width * height * sizeof(double));

    convertUcharToDoubleGrayscale(input_grayscale, width, height, double_input_grayscale);
    GaussianDerivativeX(double_input_grayscale, width, height, input_grayscale_DX_squared, sigma);
    GaussianDerivativeY(double_input_grayscale, width, height, input_grayscale_DY_squared, sigma);

    pitch = ALIGN_TO_FOUR(width);

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_DX_squared = input_grayscale_DX_squared + width * j + i;
            p_DY_squared = input_grayscale_DY_squared + width * j + i;
            p_DXDY = input_grayscale_DXDY + width * j + i;

            *p_DXDY = (*p_DX_squared) * (*p_DX_squared);
            *p_DX_squared = (*p_DX_squared) * (*p_DX_squared);
            *p_DY_squared = (*p_DY_squared) * (*p_DY_squared);
        }
    }

    GaussianWindow(input_grayscale_DX_squared, width, height, s_x2, sigma_w);
    GaussianWindow(input_grayscale_DY_squared, width, height, s_y2, sigma_w);
    GaussianWindow(input_grayscale_DXDY, width, height, s_xy, sigma_w);

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            p_sx2 = s_x2 + width * j + i;
            p_sy2 = s_y2 + width * j + i;
            p_sxy = s_xy + width * j + i;
            p_harris_corner_response_image = harris_corner_response_image + width * j + i;

            harris_corner_response = (((*p_sx2) * (*p_sy2)) - ((*p_sxy) * (*p_sxy))) - k * ((*p_sx2)  + (*p_sy2)) * ((*p_sx2)  + (*p_sy2));

            *p_harris_corner_response_image = 0;

            if (harris_corner_response > threshold) {
                *p_harris_corner_response_image = harris_corner_response;
            }
        }
    }

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            double* nbrs[8];
            p_harris_corner_response_image = harris_corner_response_image + width * j + i;
            p_output = output_grayscale + pitch * j + i;
            nbrs[0] = p_harris_corner_response_image + width;
            nbrs[1] = p_harris_corner_response_image - width;
            nbrs[2] = p_harris_corner_response_image - 1;
            nbrs[3] = p_harris_corner_response_image + 1;
            nbrs[4] = p_harris_corner_response_image + width + 1;
            nbrs[5] = p_harris_corner_response_image + width - 1;
            nbrs[6] = p_harris_corner_response_image - width + 1;
            nbrs[7] = p_harris_corner_response_image - width - 1;
            if((*p_harris_corner_response_image > *nbrs[0]) && (*p_harris_corner_response_image > *nbrs[1]) && (*p_harris_corner_response_image > *nbrs[2]) &&
                (*p_harris_corner_response_image > *nbrs[3]) && (*p_harris_corner_response_image > *nbrs[4]) && (*p_harris_corner_response_image > *nbrs[5]) &&
                (*p_harris_corner_response_image > *nbrs[6]) && (*p_harris_corner_response_image > *nbrs[7])) {
                *p_output = 255;
            }
            else {
                *p_output = 0;
            }
        }
    }

    free(input_grayscale_DX_squared);
    free(input_grayscale_DY_squared);
    free(input_grayscale_DXDY);
    free(s_x2);
    free(s_y2);
    free(s_xy);
    free(double_input_grayscale);
    free(harris_corner_response_image);

    return 0;
}

int GaussianBlur(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, double sigma)
{
    double* kernel;
    int max, kernelSize;
    double* doubleInputGrayscale;
    double* tempGrayscaleV;
    double* tempGrayscaleH;

    doubleInputGrayscale = (double*)malloc(width * height * sizeof(double));
    tempGrayscaleV = (double*)malloc(width * height * sizeof(double));
    tempGrayscaleH = (double*)malloc(width * height * sizeof(double));

    max = (int) (3*sigma);
    kernelSize = 2 * max + 1;

    kernel = (double*)malloc(kernelSize * sizeof(double));
    getGaussianKernel1D(kernel, sigma, kernelSize);

    convertUcharToDoubleGrayscale(inputGrayscale, width, height, doubleInputGrayscale);
    convolve2Dwith1Dkernel(kernel, kernelSize, doubleInputGrayscale, width, height, tempGrayscaleV, VERTICAL);
    convolve2Dwith1Dkernel(kernel, kernelSize, tempGrayscaleV, width, height, tempGrayscaleH, HORIZONTAL);
    convertDoubleToUcharGrayscale(tempGrayscaleH, width, height, outputGrayscale);

    free(kernel);
    free(tempGrayscaleV);
    free(tempGrayscaleH);
    free(doubleInputGrayscale);

    return 0;
}

int GaussianWindow(double* inputGrayscale, int width, int height, double* outputGrayscale, double sigma)
{
    double* kernel;
    int max, kernelSize;
    double* tempGrayscaleV;

    tempGrayscaleV = (double*)malloc(width * height * sizeof(double));

    max = (int) (3*sigma);
    kernelSize = 2 * max + 1;

    kernel = (double*)malloc(kernelSize * sizeof(double));
    getGaussianKernel1D(kernel, sigma, kernelSize);

    convolve2Dwith1Dkernel(kernel, kernelSize, inputGrayscale, width, height, tempGrayscaleV, VERTICAL);
    convolve2Dwith1Dkernel(kernel, kernelSize, tempGrayscaleV, width, height, outputGrayscale, HORIZONTAL);

    free(kernel);
    free(tempGrayscaleV);

    return 0;
}

void convertDoubleToUcharGrayscale(double* inputGrayscale, int width, int height, unsigned char* outputGrayscale)
{
    int i, j, pitch;
    double* pInput;
    unsigned char* pOutput;

    pitch = ALIGN_TO_FOUR(width);

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            pInput = inputGrayscale + j*width + i;
            pOutput = outputGrayscale + j*pitch + i;
            if (*pInput <= 0) {
                *pOutput = 0;
            } else if (*pInput >= 255) {
                *pOutput = 255;
            } else {
                *pOutput = (unsigned char)(*pInput);
            }
        }
    }
}

void convertUcharToDoubleGrayscale(unsigned char* inputGrayscale, int width, int height, double* outputGrayscale)
{
    int i, j, pitch;
    unsigned char* pInput;
    double* pOutput;

    pitch = ALIGN_TO_FOUR(width);

    for(j=0; j < height; j++) {
        for(i=0; i < width; i++) {
            pInput = inputGrayscale + j*pitch + i;
            pOutput = outputGrayscale + j*width + i;
            *pOutput = (double)(*pInput);
        }
    }
}
