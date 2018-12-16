#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

#include "camera.h"



void usage(FILE *fp, int argc, char **argv, char* dev_name, int frame_count)
{
        fprintf(fp,
                 "Usage: %s [options]\n"
                 "Version 1.3\n"
                 "Options:\n"
                 "-d  | --device name   Video device name [%s]n\n"
                 "-h  | --help          Print this messagen\n"
                 "-m  | --mmap          Use memory mapped buffers [default]n\n"
                 "-r  | --read          Use read() callsn\n"
                 "-u  | --userp         Use application allocated buffersn\n"
                 "-o  | --output        Outputs stream to stdoutn\n"
                 "-f  | --format        Force format to 640x480 YUYVn\n"
                 "-c  | --count         Number of frames to grab [%i]n\n"
                 "-w  | --window        Crop window\n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruo:fc:w:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", required_argument, NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { "window",  required_argument, NULL, 'w' },
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    enum io_method io_selection = IO_METHOD_MMAP;
    int device_handle = -1;
    buffers buffs;
    int frame_count = 70;
    static char *dev_name = "/dev/video0";
    static char *output_filestring = "test";
    int window_coords[4] = {100, 100, 200, 200};
    int coord_i = 0;
    char* window_args;
    int force_format = 0;
    crop_window c_window;

    for (;;) {
        int idx;
        int c;

        c = getopt_long(argc, argv,
                        short_options, long_options, &idx);

        if (-1 == c)
                break;

        switch (c) {
        case 0: /* getopt_long() flag */
                break;

        case 'd':
                dev_name = optarg;
                break;

        case 'h':
                usage(stdout, argc, argv, dev_name, frame_count);
                exit(EXIT_SUCCESS);

        case 'm':
                io_selection = IO_METHOD_MMAP;
                break;

        case 'r':
                io_selection = IO_METHOD_READ;
                break;

        case 'u':
                io_selection = IO_METHOD_USERPTR;
                break;

        case 'o':
                output_filestring = optarg;
                break;

        case 'f':
                force_format++;
                break;

        case 'c':
                errno = 0;
                frame_count = strtol(optarg, NULL, 0);
                if (errno)
                        errno_exit(optarg);
                break;
        case 'w':
            window_args = strtok(optarg, ",");
            while(window_args != NULL) {
                window_coords[coord_i++] = strtol(window_args, NULL, 0);
                window_args = strtok(NULL, ",");
            }
            break;

        default:
                usage(stderr, argc, argv, dev_name, frame_count);
                exit(EXIT_FAILURE);
        }
    }

    c_window.start_x = window_coords[0];
    c_window.start_y = window_coords[1];
    c_window.end_x = window_coords[2];
    c_window.end_y = window_coords[3];

    device_handle = open_device(dev_name);
    print_formats(device_handle);
    buffs = init_device(dev_name, device_handle, io_selection, force_format);
    start_capturing(device_handle, buffs);
    mainloop(device_handle, buffs, frame_count, output_filestring, c_window);
    stop_capturing(device_handle, buffs);
    uninit_device(buffs);
    close_device(device_handle);
    fprintf(stderr, "\n");
    return 0;
}
