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


MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(test_grayscale_writer);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
