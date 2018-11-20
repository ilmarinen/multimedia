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
} buffers;


void errno_exit(const char *s);
int xioctl(int fh, int request, void *arg);
void process_image(const void *p, int size, char* output_filestring, int frame_number, crop_window c_window);
int read_frame(int device_handle, enum io_method io_selection, buffers buffs,
                      char* output_filestring, int frame_number, crop_window c_window);
void mainloop(int device_handle, enum io_method io_selection, buffers buffs, int frame_count, char* output_filestring,
                     crop_window c_window);
void stop_capturing(int device_handle, enum io_method io_selection);
void start_capturing(int device_handle, enum io_method io_selection, buffers buffs);
void uninit_device(enum io_method io_selection, buffers buffs);
buffers init_read(unsigned int buffer_size);
buffers init_mmap(char* dev_name, int device_handle);
buffers init_userp(char* dev_name, int device_handle, unsigned int buffer_size);
buffers init_device(char* dev_name, int device_handle, enum io_method io_selection, int force_format);
void close_device(int device_handle);
void print_formats(int device_handle);
int open_device(char *device_name);

#endif
