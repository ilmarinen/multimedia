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

int makeZeroPaddedImage(unsigned char *inputGrayscale, int inputWidth, int inputHeight, int padWidth, unsigned char *outputGrayscale,
                        enum direction ptype)
{
    unsigned int i, j;
    unsigned int pitchInputGrayscale, pitchOutputGrayscale;
    unsigned char* pMovInputGrayscale;
    unsigned char* pMovOutputGrayscale;

    pMovInputGrayscale = inputGrayscale;
    pMovOutputGrayscale = outputGrayscale;

    if (ptype == VERTICAL_AND_HORIZONTAL) {
        pitchInputGrayscale = ALIGN_TO_FOUR(inputWidth);
        pitchOutputGrayscale = ALIGN_TO_FOUR(inputWidth + 2*padWidth);

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
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }

        for(j=(padWidth + inputHeight); j < (inputHeight + 2*padWidth); j++) {
            for(i=0; i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }

        for(j=0; j < (2*padWidth + inputHeight); j++) {
            for(i=0; i < padWidth; i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }

        for(j=0; j < (2*padWidth + inputHeight); j++) {
            for(i=(padWidth + inputWidth); i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }
    } else if (ptype == VERTICAL) {
        pitchInputGrayscale = ALIGN_TO_FOUR(inputWidth);
        pitchOutputGrayscale = ALIGN_TO_FOUR(inputWidth);

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
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }

        for(j=(padWidth + inputHeight); j < (inputHeight + 2*padWidth); j++) {
            for(i=0; i < inputWidth; i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }
    } else if (ptype == HORIZONTAL) {
        pitchInputGrayscale = ALIGN_TO_FOUR(inputWidth);
        pitchOutputGrayscale = ALIGN_TO_FOUR(inputWidth + 2*padWidth);

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
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }

        for(j=0; j < inputHeight; j++) {
            for(i=(padWidth + inputWidth); i < (inputWidth + 2*padWidth); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + i;
                *pMovOutputGrayscale = (unsigned char)0;
            }
        }
    }

    return 0;
}

int convolve2D(float* kernel, int kernelSize, unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale)
{
    unsigned int padWidth;
    unsigned int i, j, k, l;
    unsigned int pitchPaddedGrayscale, pitchOutputGrayscale;
    float result;

    unsigned char* paddedGrayscale;
    unsigned char* pMovPaddedGrayscale;
    unsigned char* pMovOutputGrayscale;
    float* pMovKernel;

    padWidth = (kernelSize - 1) / 2;

    paddedGrayscale = (unsigned char*)malloc(ALIGN_TO_FOUR(ALIGN_TO_FOUR(width + 2*padWidth)*(height + 2*padWidth)));

    makeZeroPaddedImage(inputGrayscale, width, height, padWidth, paddedGrayscale, VERTICAL_AND_HORIZONTAL);

    pitchPaddedGrayscale = ALIGN_TO_FOUR(width + 2*padWidth);
    pitchOutputGrayscale = ALIGN_TO_FOUR(width);

    for(j=padWidth; j < (padWidth + height); j++) {
        for(i=padWidth; i < (padWidth + width); i++) {
            pMovOutputGrayscale = outputGrayscale + (j - padWidth)*pitchOutputGrayscale + (i - padWidth);
            pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + i;
            result = 0.0;
            for(k=0; k < kernelSize; k++) {
                for(l=0; l < kernelSize; l++) {
                    pMovKernel = kernel + (kernelSize - k - 1)*kernelSize + (kernelSize - l - 1);
                    pMovPaddedGrayscale = paddedGrayscale + (j - padWidth + k)*pitchPaddedGrayscale + (i - padWidth + l);
                    result = result + (*pMovKernel) * (float)(*pMovPaddedGrayscale);
                }
            }

            if (result > 255) result = 255;
            else if (result < 0) result = 0;

            *pMovOutputGrayscale = (unsigned char)result;
        }
    }

    free(paddedGrayscale);

    return 0;
}

int convolve2Dwith1Dkernel(float* kernel, int kernelSize, unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, enum direction dir)
{
    unsigned int padWidth;
    unsigned int i, j, k;
    unsigned int pitchPaddedGrayscale, pitchOutputGrayscale;
    float result;

    unsigned char* paddedGrayscale;
    unsigned char* pMovPaddedGrayscale;
    unsigned char* pMovOutputGrayscale;
    float* pMovKernel;

    padWidth = (kernelSize - 1) / 2;

    if (dir == VERTICAL) {
        paddedGrayscale = (unsigned char*)malloc(ALIGN_TO_FOUR(ALIGN_TO_FOUR(width)*(height + 2*padWidth)));
        makeZeroPaddedImage(inputGrayscale, width, height, padWidth, paddedGrayscale, VERTICAL);

        pitchPaddedGrayscale = ALIGN_TO_FOUR(width);
        pitchOutputGrayscale = ALIGN_TO_FOUR(width);

        for(j=padWidth; j < (padWidth + height); j++) {
            for(i=0; i < width; i++) {
                pMovOutputGrayscale = outputGrayscale + (j - padWidth)*pitchOutputGrayscale + i;
                pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + i;
                result = 0.0;
                for(k=0; k < kernelSize; k++) {
                    pMovKernel = kernel + (kernelSize - k - 1);
                    pMovPaddedGrayscale = paddedGrayscale + (j - padWidth + k)*pitchPaddedGrayscale + i;
                    result = result + (*pMovKernel) * (float)(*pMovPaddedGrayscale);
                }

                if (result > 255) result = 255;
                else if (result < 0) result = 0;

                *pMovOutputGrayscale = (unsigned char)result;
            }
        }

        free(paddedGrayscale);

        return 0;

    } else if (dir == HORIZONTAL) {
        paddedGrayscale = (unsigned char*)malloc(ALIGN_TO_FOUR(ALIGN_TO_FOUR(width + 2*padWidth)*height));
        makeZeroPaddedImage(inputGrayscale, width, height, padWidth, paddedGrayscale, HORIZONTAL);

        pitchPaddedGrayscale = ALIGN_TO_FOUR(width + 2*padWidth);
        pitchOutputGrayscale = ALIGN_TO_FOUR(width);

        for(j=0; j < height; j++) {
            for(i=padWidth; i < (padWidth + width); i++) {
                pMovOutputGrayscale = outputGrayscale + j*pitchOutputGrayscale + (i - padWidth);
                pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + i;
                result = 0.0;
                for(k=0; k < kernelSize; k++) {
                    pMovKernel = kernel + (kernelSize - k - 1);
                    pMovPaddedGrayscale = paddedGrayscale + j*pitchPaddedGrayscale + (i - padWidth + k);
                    result = result + (*pMovKernel) * (float)(*pMovPaddedGrayscale);
                }

                if (result > 255) result = 255;
                else if (result < 0) result = 0;

                *pMovOutputGrayscale = (unsigned char)result;
            }
        }

        free(paddedGrayscale);

        return 0;
    }

    return -1;
}

int UniformBlur(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale)
{
    float kernel[9] =  {
        1/9.0, 1/9.0, 1/9.0,
        1/9.0, 1/9.0, 1/9.0,
        1/9.0, 1/9.0, 1/9.0
    };

    convolve2D(kernel, 3, inputGrayscale, width, height, outputGrayscale);

    return 0;
}

void getGaussianKernel1D(float* kernel, float sigma, int kernelSize)
{
    int i, max;
    float value, sum;

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

int GaussianBlur(unsigned char* inputGrayscale, int width, int height, unsigned char* outputGrayscale, float sigma)
{
    float* kernel;
    int max, kernelSize;

    max = (int) (3*sigma);
    kernelSize = 2 * max + 1;

    kernel = (float*)malloc(kernelSize * sizeof(float));
    getGaussianKernel1D(kernel, sigma, kernelSize);

    convolve2Dwith1Dkernel(kernel, kernelSize, inputGrayscale, width, height, outputGrayscale, VERTICAL);
    convolve2Dwith1Dkernel(kernel, kernelSize, inputGrayscale, width, height, outputGrayscale, HORIZONTAL);

    free(kernel);

    return 0;
}

