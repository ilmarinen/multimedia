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

#define FOUR             (4) 
#define ALIGN_TO_FOUR(VAL)        (((VAL) + FOUR - 1) & ~(FOUR - 1) )

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define R_DIFF(VV)    ( (VV) + ( 103*(VV) >> 8 ) )
#define G_DIFF(UU, VV)   (-( (88*(UU) ) >> 8) - ( (VV*183) >> 8 )  )
#define B_DIFF(UU)    ((UU) + ( (198*(UU) ) >>8 ) )

#define Y_PLUS_RDIFF(YY, VV)   ( (YY) + R_DIFF(VV) )             
#define Y_PLUS_GDIFF(YY, UU, VV)  ( (YY) + G_DIFF(UU, VV) )
#define Y_PLUS_BDIFF(YY, UU)   ( (YY) + B_DIFF(UU) )

#define EVEN_ZERO_ODD_ONE(XX)   ( ( (XX) & (0x01) ) )

/*if X > 255, X = 255; if X< 0, X = 0*/
#define CLIP1(XX)    ((unsigned char)( (XX & ~255) ? (~XX >> 15) : XX ))


enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct buffer {
        void   *start;
        size_t  length;
};

static unsigned int     n_buffers;

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

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

int YUYV2RGB24(unsigned char *pYUYV, int width, int height, unsigned char *pRGB24)
{
    unsigned int i, j;

    unsigned char *pMovY, *pMovU, *pMovV;
    unsigned char *pMovRGB;

    unsigned int pitch;
    unsigned int pitchRGB;

    pitch = 2*width;
    pitchRGB = ALIGN_TO_FOUR(3*width);


    for(j = 0; j< height; j++){
        pMovY = pYUYV + j*pitch;
        pMovU = pMovY + 1; 
        pMovV = pMovY + 3; 

        pMovRGB = pRGB24 + pitchRGB*j;

        for(i = 0; i< width; i++){
            int R, G, B;  
            int Y, U, V;

            Y = pMovY[0]; U = *pMovU - 128; V = *pMovV - 128;   
            R = Y_PLUS_RDIFF(Y, V); G = Y_PLUS_GDIFF(Y, U, V); B = Y_PLUS_BDIFF(Y, U);

            *pMovRGB = CLIP1(B);
            *(pMovRGB + 1) = CLIP1(G);     
            *(pMovRGB + 2) = CLIP1(R);   

            pMovY += 2;

            pMovU += 4*EVEN_ZERO_ODD_ONE(i);
            pMovV += 4*EVEN_ZERO_ODD_ONE(i);

            pMovRGB += 3;
        }
    }

    return 0;
}

static void process_image(const void *p, int size, char* output_filestring)
{
    unsigned char *pRGB24;
    pRGB24 = (unsigned char*)malloc(ALIGN_TO_FOUR(3*640)*480);

    YUYV2RGB24((unsigned char *)p, 640, 480, pRGB24);
    BMPwriter(pRGB24, 24, 640, 480, output_filestring);
    free(pRGB24);

    fflush(stderr);
    fprintf(stderr, ".");
}

static int read_frame(int device_handle, enum io_method io_selection, struct buffer* buffers, char* output_filestring)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (io_selection) {
        case IO_METHOD_READ:
                if (-1 == read(device_handle, buffers[0].start, buffers[0].length)) {
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

                process_image(buffers[0].start, buffers[0].length, output_filestring);
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

                assert(buf.index < n_buffers);

                process_image(buffers[buf.index].start, buf.bytesused, output_filestring);

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

                for (i = 0; i < n_buffers; ++i)
                        if (buf.m.userptr == (unsigned long)buffers[i].start
                            && buf.length == buffers[i].length)
                                break;

                assert(i < n_buffers);

                process_image((void *)buf.m.userptr, buf.bytesused, output_filestring);

                if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;
        }

        return 1;
}

static void mainloop(int device_handle, enum io_method io_selection, struct buffer* buffers, int frame_count, char* output_filestring)
{
    unsigned int count = 0;
    char output_filename[50];

    while (count < frame_count) {
        for (;;) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(device_handle, &fds);

            /* Timeout. */
            tv.tv_sec = 2;
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

            sprintf(output_filename, "%s-%d.bmp", output_filestring, count);
            if (read_frame(device_handle, io_selection, buffers, output_filename))
                    break;
            /* EAGAIN - continue select loop. */
        }
        count++;
    }
}

static void stop_capturing(int device_handle, enum io_method io_selection)
{
        enum v4l2_buf_type type;

        switch (io_selection) {
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

static void start_capturing(int device_handle, enum io_method io_selection, struct buffer* buffers)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io_selection) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
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
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_USERPTR;
            buf.index = i;
            buf.m.userptr = (unsigned long)buffers[i].start;
            buf.length = buffers[i].length;

            if (-1 == xioctl(device_handle, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(device_handle, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
        break;
    }
}

static void uninit_device(enum io_method io_selection, struct buffer* buffers)
{
        unsigned int i;

        switch (io_selection) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                        if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}

struct buffer* init_read(unsigned int buffer_size)
{
    struct buffer* buffers;

    buffers = calloc(1, sizeof(*buffers));

    if (!buffers) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc(buffer_size);

    if (!buffers[0].start) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    return buffers;
}

struct buffer* init_mmap(char* dev_name, int device_handle)
{
    struct buffer* buffers;
    struct v4l2_requestbuffers req;

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

    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
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

            buffers[n_buffers].length = buf.length;
            buffers[n_buffers].start =
                    mmap(NULL /* start anywhere */,
                          buf.length,
                          PROT_READ | PROT_WRITE /* required */,
                          MAP_SHARED /* recommended */,
                          device_handle, buf.m.offset);

            if (MAP_FAILED == buffers[n_buffers].start)
                    errno_exit("mmap");
    }

    return buffers;
}

struct buffer* init_userp(char* dev_name, int device_handle, unsigned int buffer_size)
{
    struct buffer* buffers;
    struct v4l2_requestbuffers req;

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

    buffers = calloc(4, sizeof(*buffers));

    if (!buffers) {
            fprintf(stderr, "Out of memory\\n");
            exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
            buffers[n_buffers].length = buffer_size;
            buffers[n_buffers].start = malloc(buffer_size);

            if (!buffers[n_buffers].start) {
                    fprintf(stderr, "Out of memory\\n");
                    exit(EXIT_FAILURE);
            }
    }

    return buffers;
}

struct buffer* init_device(char* dev_name, int device_handle, enum io_method io_selection, int force_format)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;
    struct buffer* buffers = NULL;

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
        fprintf(stdout, "Format: %d\n", fmt.fmt.pix.pixelformat);

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
        buffers = init_read(fmt.fmt.pix.sizeimage);
        break;

    case IO_METHOD_MMAP:
        buffers = init_mmap(dev_name, device_handle);
        break;

    case IO_METHOD_USERPTR:
        buffers = init_userp(dev_name, device_handle, fmt.fmt.pix.sizeimage);
        break;
    }

    return buffers;
}

static void close_device(int device_handle)
{
    if (-1 == close(device_handle))
        errno_exit("close");

    device_handle = -1;
}

static void print_formats(int device_handle)
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
};

static int open_device(char *device_name)
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

static void usage(FILE *fp, int argc, char **argv, char* dev_name, int frame_count)
{
        fprintf(fp,
                 "Usage: %s [options]\\n\\n"
                 "Version 1.3\\n"
                 "Options:\\n"
                 "-d | --device name   Video device name [%s]n"
                 "-h | --help          Print this messagen"
                 "-m | --mmap          Use memory mapped buffers [default]n"
                 "-r | --read          Use read() callsn"
                 "-u | --userp         Use application allocated buffersn"
                 "-o | --output        Outputs stream to stdoutn"
                 "-f | --format        Force format to 640x480 YUYVn"
                 "-c | --count         Number of frames to grab [%i]n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruo:fc:";

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
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
    enum io_method io_selection = IO_METHOD_MMAP;
    int device_handle = -1;
    struct buffer *buffers;
    int frame_count = 70;
    static char *dev_name = "/dev/video0";
    static char *output_filestring = "test";
    int force_format = 0;

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

        default:
                usage(stderr, argc, argv, dev_name, frame_count);
                exit(EXIT_FAILURE);
        }
    }

    device_handle = open_device(dev_name);
    print_formats(device_handle);
    buffers = init_device(dev_name, device_handle, io_selection, force_format);
    start_capturing(device_handle, io_selection, buffers);
    mainloop(device_handle, io_selection, buffers, frame_count, output_filestring);
    stop_capturing(device_handle, io_selection);
    uninit_device(io_selection, buffers);
    close_device(device_handle);
    fprintf(stderr, "\\n");
    return 0;
}
