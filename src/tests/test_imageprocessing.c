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
        unsigned char* pGrayscalePadded;
        unsigned char pixels[16] = {
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3
        };

        unsigned char pixelsPadded[36] = {
            0, 0, 0, 0, 0, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 3, 3, 3, 3, 0,
            0, 0, 0, 0, 0, 0
        };

        pImage = (unsigned char*)malloc(4*4);
        pGrayscalePadded = (unsigned char*)malloc(ALIGN_TO_FOUR(4 + 2*1)*6);

        imagePitch = 4;
        paddedImagePitch = ALIGN_TO_FOUR(4 + 2*1);

        memcpy(pImage, pixels, 16);
        makeZeroPaddedImage(pImage, 4, 4, 1, pGrayscalePadded);

        for(j=0; j < 6; j++) {
            for(i=0; i < 6; i++) {
                mu_check(*(pixelsPadded + 6*j + i) == *(pGrayscalePadded + j*paddedImagePitch + i));
            }
        }

        free(pImage);
        free(pGrayscalePadded);
}


MU_TEST(test_gaussian_blur) {
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
        GaussianBlur(pImage, 4, 4, pBlurredImage);

        for(j=0; j < 4; j++) {
            for(i=0; i < 4; i++) {
                mu_check(*(pixelsBlurred + imagePitch*j + i) == *(pBlurredImage + j*imagePitch + i));
            }
        }

        free(pImage);
        free(pBlurredImage);

}

MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(test_grayscale_writer);
    MU_RUN_TEST(test_padded_grayscale);
    MU_RUN_TEST(test_gaussian_blur);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
