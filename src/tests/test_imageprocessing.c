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
        unsigned char* pImage;
        unsigned char* pGrayscalePaddedVandH;
        unsigned char* pGrayscalePaddedV;
        unsigned char* pGrayscalePaddedH;
        unsigned char pixels[16] = {
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3
        };

        unsigned char pixelsPaddedVandH[36] = {
            0, 0, 0, 0, 0, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 0, 0, 0, 0, 0
        };

        unsigned char pixelsPaddedV[24] = {
            0, 0, 0, 0,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            0, 0, 0, 0,
        };

        unsigned char pixelsPaddedH[24] = {
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
        };

        pImage = (unsigned char*)malloc(4*4);
        pGrayscalePaddedVandH = (unsigned char*)malloc(ALIGN_TO_FOUR(4 + 2*1)*6);

        imagePitch = 4;
        paddedImagePitch = ALIGN_TO_FOUR(4 + 2*1);

        memcpy(pImage, pixels, 16);
        makeZeroPaddedImage(pImage, 4, 4, 1, pGrayscalePaddedVandH, VERTICAL_AND_HORIZONTAL);

        for(j=0; j < 6; j++) {
            for(i=0; i < 6; i++) {
                mu_check(*(pixelsPaddedVandH + 6*j + i) == *(pGrayscalePaddedVandH + j*paddedImagePitch + i));
            }
        }

        pGrayscalePaddedV = (unsigned char*)malloc(ALIGN_TO_FOUR(4)*6);

        imagePitch = 4;
        paddedImagePitch = ALIGN_TO_FOUR(4);

        memcpy(pImage, pixels, 16);
        makeZeroPaddedImage(pImage, 4, 4, 1, pGrayscalePaddedV, VERTICAL);

        for(j=0; j < 6; j++) {
            for(i=0; i < 4; i++) {
                mu_check(*(pixelsPaddedV + 4*j + i) == *(pGrayscalePaddedV + j*paddedImagePitch + i));
            }
        }

        pGrayscalePaddedH = (unsigned char*)malloc(ALIGN_TO_FOUR(4 + 2*1)*4);

        imagePitch = 4;
        paddedImagePitch = ALIGN_TO_FOUR(4 + 2*1);

        memcpy(pImage, pixels, 16);
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
                mu_check(*(pixelsBlurred + imagePitch*j + i) == *(pBlurredImage + j*imagePitch + i));
            }
        }

        free(pImage);
        free(pBlurredImage);

}

MU_TEST(test_convolve2dwith1dkernel) {
        int i, j;
        int imagePitch, paddedImagePitch;
        unsigned char* pImage;
        unsigned char* pBlurredImage;

        float kernel[3] = {1.0, 1.0, 1.0};

        unsigned char pixels[16] = {
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3
        };

        unsigned char pixelsBlurredV[16] = {
            6, 6, 6, 6,
            9, 9, 9, 9,
            9, 9, 9, 9,
            6, 6, 6, 6
        };

        unsigned char pixelsBlurredH[16] = {
            6, 9, 9, 6,
            6, 9, 9, 6,
            6, 9, 9, 6,
            6, 9, 9, 6
        };

        pImage = (unsigned char*)malloc(4*4);
        pBlurredImage = (unsigned char*)malloc(4*4);

        imagePitch = 4;

        memcpy(pImage, pixels, 16);

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
    float* kernel;

    kernel = (float *)malloc(5 * sizeof(float));

    getGaussianKernel1D(kernel, 1.0, 5);

    mu_check(abs(kernel[2] - 0.398) < 0.0001);

    free(kernel);
}

MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(test_grayscale_writer);
    MU_RUN_TEST(test_padded_grayscale);
    MU_RUN_TEST(test_uniform_blur);
    MU_RUN_TEST(test_convolve2dwith1dkernel);
    MU_RUN_TEST(test_getGaussianKernel1d);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
