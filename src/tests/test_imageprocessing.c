#include "minunit.h"

#include "imageprocessing.h"


void test_setup(void) {
    /* Nothing */
}

void test_teardown(void) {
    /* Nothing */
}


MU_TEST(test_grayscale_writer) {
    unsigned char* pImage;
    FILE *fp;
    long size = 0;

    pImage = (unsigned char*)malloc(4*4);
    GrayScaleWriter(pImage, 4, 4, "test-grayscale.bmp");
    free(pImage);

    fp = fopen("test-grayscale.bmp", "rb");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    remove("test-grayscale.bmp");

    mu_check(size == (54 + 256 * sizeof(RGBQUAD) + 16));
}


MU_TEST(test_padded_grayscale) {
        int i, j;
        int imagePitch, paddedImagePitch;
        double* pImage;
        double* pGrayscalePaddedVandH;
        double* pGrayscalePaddedV;
        double* pGrayscalePaddedH;
        double pixels[16] = {
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0
        };

        double pixelsPaddedVandH[36] = {
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        };

        double pixelsPaddedV[24] = {
            0.0, 0.0, 0.0, 0.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            0.0, 0.0, 0.0, 0.0,
        };

        double pixelsPaddedH[24] = {
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
            0.0, 3.0, 3.0, 3.0, 3.0, 0.0,
        };

        pImage = (double*)malloc(4*4*sizeof(double));
        pGrayscalePaddedVandH = (double*)malloc(6*6*sizeof(double));

        imagePitch = 4;
        paddedImagePitch = 4 + 2*1;

        memcpy(pImage, pixels, 16*sizeof(double));
        makeZeroPaddedImage(pImage, 4, 4, 1, pGrayscalePaddedVandH, VERTICAL_AND_HORIZONTAL);

        for(j=0; j < 6; j++) {
            for(i=0; i < 6; i++) {
                mu_check(*(pixelsPaddedVandH + 6*j + i) == *(pGrayscalePaddedVandH + j*paddedImagePitch + i));
            }
        }

        pGrayscalePaddedV = (double*)malloc(4*6*sizeof(double));

        imagePitch = 4;
        paddedImagePitch = 4;

        memcpy(pImage, pixels, 16*sizeof(double));
        makeZeroPaddedImage(pImage, 4, 4, 1, pGrayscalePaddedV, VERTICAL);

        for(j=0; j < 6; j++) {
            for(i=0; i < 4; i++) {
                mu_check(*(pixelsPaddedV + 4*j + i) == *(pGrayscalePaddedV + j*paddedImagePitch + i));
            }
        }

        pGrayscalePaddedH = (double*)malloc((4 + 2*1)*4*sizeof(double));

        imagePitch = 4;
        paddedImagePitch = 4 + 2*1;

        memcpy(pImage, pixels, 16*sizeof(double));
        makeZeroPaddedImage(pImage, 4, 4, 1, pGrayscalePaddedH, HORIZONTAL);

        for(j=0; j < 4; j++) {
            for(i=0; i < 6; i++) {
                mu_check(*(pixelsPaddedH + 6*j + i) == *(pGrayscalePaddedH + j*paddedImagePitch + i));
            }
        }

        free(pImage);
        free(pGrayscalePaddedVandH);
        free(pGrayscalePaddedV);
        free(pGrayscalePaddedH);
}


MU_TEST(test_uniform_blur) {
        int i, j;
        int imagePitch, paddedImagePitch;
        unsigned char* pImage;
        unsigned char* pBlurredImage;
        unsigned char pixels[16] = {
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3
        };

        unsigned char pixelsBlurred[36] = {
            1, 2, 2, 1,
            2, 2, 2, 2,
            2, 2, 2, 2,
            1, 2, 2, 1
        };

        pImage = (unsigned char*)malloc(4*4);
        pBlurredImage = (unsigned char*)malloc(4*4);

        imagePitch = 4;

        memcpy(pImage, pixels, 16);
        UniformBlur(pImage, 4, 4, pBlurredImage);

        for(j=0; j < 4; j++) {
            for(i=0; i < 4; i++) {
                mu_check(abs(*(pixelsBlurred + imagePitch*j + i) - *(pBlurredImage + j*imagePitch + i)) <= 1);
            }
        }

        free(pImage);
        free(pBlurredImage);

}

MU_TEST(test_convolve2dwith1dkernel) {
        int i, j;
        int imagePitch, paddedImagePitch;
        double* pImage;
        double* pBlurredImage;

        double kernel[3] = {1.0, 1.0, 1.0};

        double pixels[16] = {
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0,
            3.0, 3.0, 3.0, 3.0
        };

        double pixelsBlurredV[16] = {
            6.0, 6.0, 6.0, 6.0,
            9.0, 9.0, 9.0, 9.0,
            9.0, 9.0, 9.0, 9.0,
            6.0, 6.0, 6.0, 6.0
        };

        double pixelsBlurredH[16] = {
            6.0, 9.0, 9.0, 6.0,
            6.0, 9.0, 9.0, 6.0,
            6.0, 9.0, 9.0, 6.0,
            6.0, 9.0, 9.0, 6.0
        };

        pImage = (double*)malloc(4*4*sizeof(double));
        pBlurredImage = (double*)malloc(4*4*sizeof(double));

        imagePitch = 4;

        memcpy(pImage, pixels, 16*sizeof(double));

        convolve2Dwith1Dkernel(kernel, 3, pImage, 4, 4, pBlurredImage, VERTICAL);

        for(j=0; j < 4; j++) {
            for(i=0; i < 4; i++) {
                mu_check(*(pixelsBlurredV + imagePitch*j + i) == *(pBlurredImage + j*imagePitch + i));
            }
        }

        convolve2Dwith1Dkernel(kernel, 3, pImage, 4, 4, pBlurredImage, HORIZONTAL);

        for(j=0; j < 4; j++) {
            for(i=0; i < 4; i++) {
                mu_check(*(pixelsBlurredH + imagePitch*j + i) == *(pBlurredImage + j*imagePitch + i));
            }
        }

        free(pImage);
        free(pBlurredImage);

}

MU_TEST(test_getGaussianKernel1d) {
    double* kernel;

    kernel = (double *)malloc(5 * sizeof(double));

    getGaussianKernel1D(kernel, 1.0, 5);

    mu_check(abs(kernel[2] - 0.398) < 0.0001);

    free(kernel);
}

MU_TEST(test_GaussianBlur) {
    double* kernel;
    unsigned char* outputGrayscale2d;
    unsigned char* outputGrayscaleSeparable;
    unsigned char* pOutput2d;
    unsigned char* pOutputSeparable;
    int i, j;

    unsigned char pixels[16] = {
        30, 30, 30, 30,
        30, 30, 30, 30,
        30, 30, 30, 30,
        30, 30, 30, 30
    };

    kernel = (double *)malloc(5 * 5 * sizeof(double));
    outputGrayscale2d = (unsigned char *)malloc(ALIGN_TO_FOUR(4) * 4);
    outputGrayscaleSeparable = (unsigned char *)malloc(ALIGN_TO_FOUR(4) * 4);

    GaussianBlur2DKernel(pixels, 4, 4, outputGrayscale2d, 1.0);
    GaussianBlur(pixels, 4, 4, outputGrayscaleSeparable, 1.0);

    for(j=0; j < 4; j++) {
        for(i=0; i < 4; i++) {
            pOutput2d = outputGrayscale2d + 4 * j + i;
            pOutputSeparable = outputGrayscaleSeparable + 4 * j + i;
            mu_check(abs(*pOutput2d - *pOutputSeparable) <= 1);
        }
    }

    free(kernel);
    free(outputGrayscale2d);
    free(outputGrayscaleSeparable);
}

MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(test_grayscale_writer);
    MU_RUN_TEST(test_padded_grayscale);
    MU_RUN_TEST(test_uniform_blur);
    MU_RUN_TEST(test_convolve2dwith1dkernel);
    MU_RUN_TEST(test_getGaussianKernel1d);
    MU_RUN_TEST(test_GaussianBlur);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
