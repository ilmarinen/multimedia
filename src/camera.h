#ifndef CAMERA_H_   /* Include guard */
#define CAMERA_H_

#include "imageprocessing.h"

enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct buffer {
        void   *start;
        size_t  length;
};

typedef struct buffers_ {
    struct buffer* buffers;
    unsigned int n_buffers;
    enum io_method io_selection;
    unsigned int image_width;
    unsigned int image_height;
} buffers;

typedef struct res_ {
    unsigned int width;
    unsigned int height;
} resolution;

void errno_exit(const char *s);
int xioctl(int fh, int request, void *arg);
void process_image(const void *p, int size, unsigned int width, unsigned int height, char* output_filestring, int frame_number, crop_window c_window);
int read_frame(int device_handle, buffers buffs,
                      char* output_filestring, int frame_number, crop_window c_window);
void mainloop(int device_handle, buffers buffs, int frame_count, char* output_filestring,
                     crop_window c_window);
int grab_frame(int device_handle, buffers buffs, unsigned char* image_buffer);
int grab_frame_rgb(char* dev_name, int width, int height, unsigned char* image_buffer);
int grab_frame_yuyv(char* device_name, int width, int height, unsigned char* image_buffer);
void stop_capturing(int device_handle, buffers buffs);
void start_capturing(int device_handle, buffers buffs);
void uninit_device(buffers buffs);
buffers init_read(unsigned int buffer_size);
buffers init_mmap(char* dev_name, int device_handle);
buffers init_userp(char* dev_name, int device_handle, unsigned int buffer_size);
buffers init_device(char* dev_name, int device_handle, enum io_method io_selection, int force_format);
void close_device(int device_handle);
void print_formats(int device_handle);
resolution get_resolution(int device_handle);
int open_device(char *device_name);

#endif
