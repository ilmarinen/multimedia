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

#include "imageprocessing.h"
#include "camera.h"


void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

void process_image(const void *p, int size, unsigned int width, unsigned int height, char* output_filestring,
                   int frame_number, crop_window c_window)
{
    char output_filename[50], output_cropped_filename[50],
         output_grayscale_filename[50], output_grayscale_padded_filename[50],
         output_blurred_uniform_filename[50], output_blurred_gaussian_filename[50],
         output_blurred_gaussian2d_filename[50], output_canny_edge_detector_filename[50], output_corner_detector_filename[50];
    unsigned char *pRGB24, *pCroppedRGB24, *pGrayscale, *pGrayscalePadded, *pGrayscaleBlurred, *pGrayscaleBlurredGaussian,
        *pGrayscaleBlurredGaussian2d, *pCannyEdges, *pCorners;
    pRGB24 = (unsigned char*)malloc(ALIGN_TO_FOUR(3*width)*height);
    pGrayscale = (unsigned char*)malloc(ALIGN_TO_FOUR(width)*height);
    pGrayscalePadded = (unsigned char*)malloc(ALIGN_TO_FOUR(ALIGN_TO_FOUR(width + 2*1)*(height + 2*1)));
    pGrayscaleBlurred = (unsigned char*)malloc(ALIGN_TO_FOUR(width)*height);
    pGrayscaleBlurredGaussian = (unsigned char*)malloc(ALIGN_TO_FOUR(width)*height);
    pGrayscaleBlurredGaussian2d = (unsigned char*)malloc(ALIGN_TO_FOUR(width)*height);
    pCannyEdges = (unsigned char*)malloc(ALIGN_TO_FOUR(width)*height);
    pCorners = (unsigned char*)malloc(ALIGN_TO_FOUR(width)*height);
    pCroppedRGB24 = (unsigned char*)malloc(ALIGN_TO_FOUR(3*(c_window.end_x - c_window.start_x))*(c_window.end_y - c_window.start_y));

    sprintf(output_filename, "%s-%d.bmp", output_filestring, frame_number);
    sprintf(output_cropped_filename, "%s-%d-cropped.bmp", output_filestring, frame_number);
    sprintf(output_grayscale_filename, "%s-%d-gray.bmp", output_filestring, frame_number);
    sprintf(output_grayscale_padded_filename, "%s-%d-gray-padded.bmp", output_filestring, frame_number);
    sprintf(output_blurred_uniform_filename, "%s-%d-gray-blurred-uniform.bmp", output_filestring, frame_number);
    sprintf(output_blurred_gaussian_filename, "%s-%d-gray-blurred-gaussian.bmp", output_filestring, frame_number);
    sprintf(output_blurred_gaussian2d_filename, "%s-%d-gray-blurred-gaussian-2d.bmp", output_filestring, frame_number);
    sprintf(output_canny_edge_detector_filename, "%s-%d-canny-edges.bmp", output_filestring, frame_number);
    sprintf(output_corner_detector_filename, "%s-%d-corners.bmp", output_filestring, frame_number);

    YUYV2RGB24((unsigned char *)p, width, height, pRGB24);
    RGB24toGrayscale(pRGB24, width, height, pGrayscale);
    makeZeroPaddedImage(pGrayscale, width, height, 1, pGrayscalePadded, VERTICAL_AND_HORIZONTAL);
    UniformBlur(pGrayscale, width, height, pGrayscaleBlurred);
    GaussianBlur2DKernel(pGrayscale, width, height, pGrayscaleBlurredGaussian2d, 1.0);
    GaussianBlur(pGrayscale, width, height, pGrayscaleBlurredGaussian, 1.0);
    DifferentialEdgeDetector(pGrayscale, width, height, pCannyEdges, 2.0, 5.0, 10.0);
    CornerDetector(pGrayscale, width, height, pCorners, 2.0, 2.0, 0.04, 500.0);

    GrayScaleWriter(pGrayscale, width, height, output_grayscale_filename);
    GrayScaleWriter(pGrayscalePadded, (width + 2), (height + 2), output_grayscale_padded_filename);
    GrayScaleWriter(pGrayscaleBlurred, width, height, output_blurred_uniform_filename);
    GrayScaleWriter(pGrayscaleBlurredGaussian, width, height, output_blurred_gaussian_filename);
    GrayScaleWriter(pGrayscaleBlurredGaussian2d, width, height, output_blurred_gaussian2d_filename);
    GrayScaleWriter(pCannyEdges, width, height, output_canny_edge_detector_filename);
    GrayScaleWriter(pCorners, width, height, output_corner_detector_filename);
    cropRGB24(pRGB24, width, height, c_window.start_x, c_window.start_y, c_window.end_x, c_window.end_y, pCroppedRGB24);
    BMPwriter(pCroppedRGB24, 24, (c_window.end_x - c_window.start_x), (c_window.end_y - c_window.start_y), output_cropped_filename);
    BMPwriter(pRGB24, 24, width, height, output_filename);
    free(pRGB24);
    free(pCroppedRGB24);
    free(pGrayscale);
    free(pGrayscalePadded);
    free(pGrayscaleBlurred);
    free(pGrayscaleBlurredGaussian);
    free(pGrayscaleBlurredGaussian2d);
    free(pCannyEdges);
    free(pCorners);

    fflush(stderr);
    fprintf(stderr, ".");
}

int read_frame(int device_handle, buffers buffs,
                      char* output_filestring, int frame_number, crop_window c_window)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (buffs.io_selection) {
        case IO_METHOD_READ:
                if (-1 == read(device_handle, buffs.buffers[0].start, buffs.buffers[0].length)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("read");
                        }
                }

                process_image(buffs.buffers[0].start, buffs.buffers[0].length, buffs.image_width, buffs.image_height,
                              output_filestring, frame_number, c_window);
                break;

        case IO_METHOD_MMAP:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (-1 == xioctl(device_handle, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                assert(buf.index < buffs.n_buffers);

                process_image(buffs.buffers[buf.index].start, buf.bytesused, buffs.image_width, buffs.image_height,
                              output_filestring, frame_number, c_window);

                if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;

        case IO_METHOD_USERPTR:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == xioctl(device_handle, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                for (i = 0; i < buffs.n_buffers; ++i)
                        if (buf.m.userptr == (unsigned long)buffs.buffers[i].start
                            && buf.length == buffs.buffers[i].length)
                                break;

                assert(i < buffs.n_buffers);

                process_image((void *)buf.m.userptr, buf.bytesused, buffs.image_width, buffs.image_height,
                              output_filestring, frame_number, c_window);

                if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;
        }

        return 1;
}

int grab_frame(int device_handle, buffers buffs, unsigned char* image_buffer)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (buffs.io_selection) {
        case IO_METHOD_READ:
                if (-1 == read(device_handle, buffs.buffers[0].start, buffs.buffers[0].length)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("read");
                        }
                }

                // process_image(buffs.buffers[0].start, buffs.buffers[0].length, buffs.image_width, buffs.image_height,
                //               output_filestring, frame_number, c_window);
                memcpy(image_buffer, buffs.buffers[0].start, buffs.buffers[0].length);

                break;

        case IO_METHOD_MMAP:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (-1 == xioctl(device_handle, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                assert(buf.index < buffs.n_buffers);

                // process_image(buffs.buffers[buf.index].start, buf.bytesused, buffs.image_width, buffs.image_height,
                //               output_filestring, frame_number, c_window);
                memcpy(image_buffer, buffs.buffers[buf.index].start, buf.bytesused);

                if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;

        case IO_METHOD_USERPTR:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == xioctl(device_handle, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                for (i = 0; i < buffs.n_buffers; ++i)
                        if (buf.m.userptr == (unsigned long)buffs.buffers[i].start
                            && buf.length == buffs.buffers[i].length)
                                break;

                assert(i < buffs.n_buffers);

                // process_image((void *)buf.m.userptr, buf.bytesused, buffs.image_width, buffs.image_height,
                //               output_filestring, frame_number, c_window);
                memcpy(image_buffer, (void *)buf.m.userptr, buf.bytesused);

                if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;
        }

        return 1;
}

int grab_frame_yuyv(char* dev_name, int width, int height, unsigned char* image_buffer)
{
    int device_handle = 0;
    buffers buffs;

    device_handle = open_device(dev_name);
    buffs = init_device(dev_name, device_handle, IO_METHOD_USERPTR, 0);
    start_capturing(device_handle, buffs);
    grab_frame(device_handle, buffs, image_buffer);
    stop_capturing(device_handle, buffs);
    uninit_device(buffs);
    close_device(device_handle);

    return 1;
}

int grab_frame_rgb(char* dev_name, int width, int height, unsigned char* image_buffer)
{
    int device_handle = 0;
    buffers buffs;
    unsigned char* image_buffer_yuyv;

    image_buffer_yuyv = (unsigned char*)malloc(width*height*2);

    device_handle = open_device(dev_name);
    buffs = init_device(dev_name, device_handle, IO_METHOD_USERPTR, 0);
    start_capturing(device_handle, buffs);

    for(;;) {
        if(grab_frame(device_handle, buffs, image_buffer_yuyv))
            break;
    }
    YUYV2RGB24(image_buffer_yuyv, width, height, image_buffer);
    stop_capturing(device_handle, buffs);
    uninit_device(buffs);
    close_device(device_handle);
    free(image_buffer_yuyv);

    return 1;
}

void mainloop(int device_handle, buffers buffs, int frame_count, char* output_filestring,
                     crop_window c_window)
{
    unsigned int count = 0;

    while (count < frame_count) {
        for (;;) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(device_handle, &fds);

            /* Timeout. */
            tv.tv_sec = 5;
            tv.tv_usec = 0;

            r = select(device_handle + 1, &fds, NULL, NULL, &tv);

            if (-1 == r) {
                    if (EINTR == errno)
                            continue;
                    errno_exit("select");
            }

            if (0 == r) {
                    fprintf(stderr, "select timeout\\n");
                    exit(EXIT_FAILURE);
            }

            if (read_frame(device_handle, buffs, output_filestring, count, c_window))
                    break;
            /* EAGAIN - continue select loop. */
        }
        count++;
    }
}

void stop_capturing(int device_handle, buffers buffs)
{
        enum v4l2_buf_type type;

        switch (buffs.io_selection) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(device_handle, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }

}

void start_capturing(int device_handle, buffers buffs)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (buffs.io_selection) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < buffs.n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(device_handle, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < buffs.n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)buffs.buffers[i].start;
            buf.length = buffs.buffers[i].length;

            if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(device_handle, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;
    }
}

void uninit_device(buffers buffs)
{
        unsigned int i;

        switch (buffs.io_selection) {
        case IO_METHOD_READ:
                free(buffs.buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < buffs.n_buffers; ++i)
                        if (-1 == munmap(buffs.buffers[i].start, buffs.buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < buffs.n_buffers; ++i)
                        free(buffs.buffers[i].start);
                break;
        }

        free(buffs.buffers);
}

buffers init_read(unsigned int buffer_size)
{
    struct buffer* pBuffers;

    pBuffers = calloc(1, sizeof(*pBuffers));

    if (!pBuffers) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    pBuffers[0].length = buffer_size;
    pBuffers[0].start = malloc(buffer_size);

    if (!pBuffers[0].start) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    buffers bufs;
    bufs.io_selection = IO_METHOD_READ;
    bufs.n_buffers = 1;
    bufs.buffers = pBuffers;

    return bufs;
}

buffers init_mmap(char* dev_name, int device_handle)
{
    struct buffer* pBuffers;
    struct v4l2_requestbuffers req;
    buffers buffs;
    unsigned int n_buffers;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(device_handle, VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                    fprintf(stderr, "%s does not support "
                             "memory mappingn", dev_name);
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }

    if (req.count < 2) {
            fprintf(stderr, "Insufficient buffer memory on %s\\n",
                     dev_name);
            exit(EXIT_FAILURE);
    }

    pBuffers = calloc(req.count, sizeof(*pBuffers));

    if (!pBuffers) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory      = V4L2_MEMORY_MMAP;
            buf.index       = n_buffers;

            if (-1 == xioctl(device_handle, VIDIOC_QUERYBUF, &buf))
                    errno_exit("VIDIOC_QUERYBUF");

            pBuffers[n_buffers].length = buf.length;
            pBuffers[n_buffers].start =
                    mmap(NULL /* start anywhere */,
                          buf.length,
                          PROT_READ | PROT_WRITE /* required */,
                          MAP_SHARED /* recommended */,
                          device_handle, buf.m.offset);

            if (MAP_FAILED == pBuffers[n_buffers].start)
                    errno_exit("mmap");
    }

    buffs.io_selection = IO_METHOD_MMAP;
    buffs.buffers = pBuffers;
    buffs.n_buffers = req.count;

    return buffs;
}

buffers init_userp(char* dev_name, int device_handle, unsigned int buffer_size)
{
    struct buffer* pBuffers;
    struct v4l2_requestbuffers req;
    buffers buffs;
    unsigned int n_buffers;

    CLEAR(req);

    req.count  = 4;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(device_handle, VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                    fprintf(stderr, "%s does not support "
                             "user pointer i/on", dev_name);
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }

    pBuffers = calloc(4, sizeof(*pBuffers));

    if (!pBuffers) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
            pBuffers[n_buffers].length = buffer_size;
            pBuffers[n_buffers].start = malloc(buffer_size);

            if (!pBuffers[n_buffers].start) {
                    fprintf(stderr, "Out of memory\\n");
                    exit(EXIT_FAILURE);
            }
    }

    buffs.io_selection = IO_METHOD_USERPTR;
    buffs.buffers = pBuffers;
    buffs.n_buffers = 4;

    return buffs;
}

buffers init_device(char* dev_name, int device_handle, enum io_method io_selection, int force_format)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;
    buffers buffs;

    if (-1 == xioctl(device_handle, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
                fprintf(stderr, "%s is no V4L2 device\\n",
                         dev_name);
                exit(EXIT_FAILURE);
        } else {
                errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\\n",
                 dev_name);
        exit(EXIT_FAILURE);
    }

    switch (io_selection) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf(stderr, "%s does not support read i/o\\n",
                     dev_name);
            exit(EXIT_FAILURE);
        }
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "%s does not support streaming i/o\\n",
                     dev_name);
            exit(EXIT_FAILURE);
        }
        break;
    }


    /* Select video input, video standard and tune here. */


    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(device_handle, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(device_handle, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                    /* Cropping not supported. */
                    break;
            default:
                    /* Errors ignored. */
                    break;
            }
        }
    } else {
            /* Errors ignored. */
    }


    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (force_format) {
        fmt.fmt.pix.width       = 640;
        fmt.fmt.pix.height      = 480;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl(device_handle, VIDIOC_S_FMT, &fmt))
            errno_exit("VIDIOC_S_FMT");
        fprintf(stdout, "Force format.\n");

        /* Note VIDIOC_S_FMT may change width and height. */
    } else {
        /* Preserve original settings as set by v4l2-ctl for example */
        if (-1 == xioctl(device_handle, VIDIOC_G_FMT, &fmt))
            errno_exit("VIDIOC_G_FMT");
    }

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    switch (io_selection) {
    case IO_METHOD_READ:
        buffs = init_read(fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        buffs = init_mmap(dev_name, device_handle);
        break;

    case IO_METHOD_USERPTR:
        buffs = init_userp(dev_name, device_handle, fmt.fmt.pix.sizeimage);
        break;
    }

    buffs.image_width = fmt.fmt.pix.width;
    buffs.image_height = fmt.fmt.pix.height;

    return buffs;
}

void close_device(int device_handle)
{
    if (-1 == close(device_handle))
        errno_exit("close");

    device_handle = -1;
}

void print_formats(int device_handle)
{
    struct v4l2_fmtdesc fmtdesc;
    CLEAR(fmtdesc);
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;
    while (xioctl(device_handle, VIDIOC_ENUM_FMT, &fmtdesc) == 0) 
    {
        fprintf(stdout, "%s\n", fmtdesc.description);
        fmtdesc.index++;
    }
}

resolution get_resolution(int device_handle)
{
    unsigned int min;
    struct v4l2_format fmt;
    resolution res;

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    /* Preserve original settings as set by v4l2-ctl for example */
    if (-1 == xioctl(device_handle, VIDIOC_G_FMT, &fmt))
        errno_exit("VIDIOC_G_FMT");

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    res.width = fmt.fmt.pix.width;
    res.height = fmt.fmt.pix.height;

    return res;
}

int open_device(char *device_name)
{
    struct stat st;
    int device_handle = -1;

    if (-1 == stat(device_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\\n",
                 device_name, errno, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no devicen", device_name);
        return -1;
    }

    device_handle = open(device_name, O_RDWR /* required */ | O_NONBLOCK, 0);

    if (-1 == device_handle) {
        fprintf(stderr, "Cannot open '%s': %d, %s\\n",
                 device_name, errno, strerror(errno));
        return -1;
    }

    return device_handle;
}
