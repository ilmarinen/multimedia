#include "minunit.h"

#include "imageprocessing.h"
#include "camera.h"


void test_setup(void) {
    /* Nothing */
}

void test_teardown(void) {
    /* Nothing */
}


MU_TEST(test_grab_frame_rgb) {
    unsigned char* pRGB;
    FILE *fp;
    resolution res;
    long size = 0;
    int dev_handle = 0;

    dev_handle = open_device("/dev/video0");
    res = get_resolution(dev_handle);
    close_device(dev_handle);

    pRGB = (unsigned char*)malloc(ALIGN_TO_FOUR(res.width*3)*res.height);
    grab_frame_rgb("/dev/video0", res.width, res.height, pRGB);
    BMPwriter(pRGB, 24, res.width, res.height, "test.bmp");

    fp = fopen("test.bmp", "rb");
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fclose(fp);
    remove("test.bmp");
    free(pRGB);

    mu_check(size == (54 + ALIGN_TO_FOUR(res.width*3)*res.height));
}


MU_TEST_SUITE(test_suite) {
    MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

    MU_RUN_TEST(test_grab_frame_rgb);
}

int main(int argc, char *argv[]) {
    MU_RUN_SUITE(test_suite);
    MU_REPORT();
    return 0;
}
