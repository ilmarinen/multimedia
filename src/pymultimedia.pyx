import math
import numpy as np
from libc.stdlib cimport malloc, free

cimport numpy as np


cdef extern from "camera.h":
    ctypedef struct resolution:
        unsigned int width
        unsigned int height
    cdef int open_device(char* device_name)
    cdef void close_device(int device_handle)
    cdef int grab_frame_rgb(char* device_name, int width, int height, unsigned char* image_buffer_rgb);
    cdef resolution get_resolution(int device_handle);


cdef extern from "imageprocessing.h":
    cdef int BMPwriter(unsigned char *pRGB, int bitNum, int width, int height, char* output_filestring);


cpdef int py_open_device(dev_name):
    cdef bytes dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes

    return open_device(d_name)


cpdef py_close_device(int dev_handle):
    close_device(dev_handle)


cpdef tuple py_get_resolution(int dev_handle):
    cdef resolution res = get_resolution(dev_handle)

    return (res.get("width"), res.get("height")) 

cpdef np.ndarray py_grab_frame_rgb(dev_name, int width, int height):
    cdef unsigned char* image_buffer_rgb
    cdef dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes
    cpdef int result

    image_buffer_rgb = <unsigned char*> malloc(int(math.ceil(width*3/4.0)*4.0)*height);
    result = grab_frame_rgb(d_name, width, height, image_buffer_rgb)

    image_numpy_array = np.ndarray((height, width, 3), dtype=np.uint8)
    pitch = int(math.ceil(width*3/4.0)*4.0)
    for i in range(width):
        for j in range(height):
            pixel_address = j*pitch + i*3
            pixel_b = image_buffer_rgb[pixel_address]
            pixel_g = image_buffer_rgb[pixel_address + 1]
            pixel_r = image_buffer_rgb[pixel_address + 2]
            image_numpy_array[j, i, 2] = int(pixel_b)
            image_numpy_array[j, i, 1] = int(pixel_g)
            image_numpy_array[j, i, 0] = int(pixel_r)


    free(image_buffer_rgb)

    return image_numpy_array
