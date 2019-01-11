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
    cdef int RGB24toGrayscale(unsigned char *inputRGB24, int width, int height, unsigned char *outputGrayscale);
    cdef int GaussianDerivativeX(double* input_grayscale, int width, int height, double* output_grayscale, double sigma);
    cdef int GaussianDerivativeY(double* input_grayscale, int width, int height, double* output_grayscale, double sigma);
    cdef void convertDoubleToUcharGrayscale(double* inputGrayscale, int width, int height, unsigned char* outputGrayscale);
    cdef void convertUcharToDoubleGrayscale(unsigned char* inputGrayscale, int width, int height, double* outputGrayscale);
    cdef int GaussianDerivativeXX(double* input_grayscale, int width, int height, double* output_grayscale, double sigma);
    cdef int GaussianDerivativeYY(double* input_grayscale, int width, int height, double* output_grayscale, double sigma);
    cdef int GaussianDerivativeXY(double* input_grayscale, int width, int height, double* output_grayscale, double sigma);
    cdef void getGaussianKernel1D(double* kernel, double sigma, int kernelSize);
    cdef void getDGausianKernel1D(double* kernel, double sigma, int kernelSize);
    cdef void getD2GausianKernel1D(double* kernel, double sigma, int kernelSize);
    cdef int CannyEdgeDetector(
        unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale,
        double sigma, double threshold, double cutoff_threshold);
    cdef int CornerDetector(
        unsigned char* input_grayscale, int width, int height, unsigned char* output_grayscale,
        double sigma, double sigma_w, double k, double threshold);


cpdef int py_open_device(dev_name):
    cdef bytes dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes

    return open_device(d_name)


cpdef py_close_device(int dev_handle):
    close_device(dev_handle)


cpdef tuple py_get_resolution(int dev_handle):
    cdef resolution res = get_resolution(dev_handle)

    return (res.get("width"), res.get("height")) 

cpdef tuple py_get_device_resolution(dev_name):
    cdef bytes dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes
    cdef int dev_handle

    dev_handle = open_device(d_name)

    cdef resolution res = get_resolution(dev_handle)

    close_device(dev_handle)

    return (res.get("width"), res.get("height")) 

cpdef np.ndarray numpy_array_from_image(unsigned char* input_image, int width, int height, image_type="rgb"):
    image_numpy_array = None
    if image_type == "rgb":
        image_numpy_array = np.ndarray((height, width, 3), dtype=np.uint8)
        pitch = int(math.ceil(width*3/4.0)*4.0)
        for i in range(width):
            for j in range(height):
                pixel_address = j*pitch + i*3
                pixel_b = input_image[pixel_address]
                pixel_g = input_image[pixel_address + 1]
                pixel_r = input_image[pixel_address + 2]
                image_numpy_array[j, i, 2] = int(pixel_b)
                image_numpy_array[j, i, 1] = int(pixel_g)
                image_numpy_array[j, i, 0] = int(pixel_r)

    elif image_type == "grayscale":
        image_numpy_array = np.ndarray((height, width), dtype=np.uint8)
        pitch = int(math.ceil(width/4.0)*4.0)
        for i in range(width):
            for j in range(height):
                pixel_address = j*pitch + i
                pixel_intensity = input_image[pixel_address]
                image_numpy_array[j, i] = int(pixel_intensity)

    return image_numpy_array


cpdef np.ndarray py_grab_frame_rgb(dev_name, int width, int height):
    cdef unsigned char* image_buffer_rgb
    cdef dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes
    cpdef int result

    image_buffer_rgb = <unsigned char*> malloc(int(math.ceil(width*3/4.0)*4.0)*height);
    result = grab_frame_rgb(d_name, width, height, image_buffer_rgb)
    image_numpy_array = numpy_array_from_image(image_buffer_rgb, width, height, image_type="rgb")

    free(image_buffer_rgb)

    return image_numpy_array

cpdef py_grab_frame_grayscale(dev_name, int width, int height, sigma=2.0):
    cdef unsigned char* image_buffer_rgb
    cdef unsigned char* image_buffer_grayscale
    cdef double* double_input_grayscale;
    cdef double* input_grayscale_DX
    cdef double* input_grayscale_DY
    cdef double* input_grayscale_DXX
    cdef double* input_grayscale_DYY
    cdef double* input_grayscale_DXY
    cdef dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes
    cpdef int result

    image_buffer_rgb = <unsigned char*> malloc(int(math.ceil(width*3/4.0)*4.0)*height);
    image_buffer_grayscale = <unsigned char*> malloc(int(math.ceil(width/4.0)*4.0)*height);

    result = grab_frame_rgb(d_name, width, height, image_buffer_rgb)
    RGB24toGrayscale(image_buffer_rgb, width, height, image_buffer_grayscale);
    image_numpy_array = numpy_array_from_image(image_buffer_grayscale, width, height, image_type="grayscale")

    free(image_buffer_rgb)
    free(image_buffer_grayscale)

    return image_numpy_array


cpdef get_image_derivatives(imagearray, sigma=2.0):
    cdef int width
    cdef int height
    cdef double* double_input_grayscale;
    cdef double* input_grayscale_DX
    cdef double* input_grayscale_DY
    cdef double* input_grayscale_DXX
    cdef double* input_grayscale_DYY
    cdef double* input_grayscale_DXY

    height, width = imagearray.shape

    double_input_grayscale = <double*> malloc(width * height * sizeof(double))
    input_grayscale_DX = <double*> malloc(width * height * sizeof(double))
    input_grayscale_DY = <double*> malloc(width * height * sizeof(double))
    input_grayscale_DXX = <double*> malloc(width * height * sizeof(double))
    input_grayscale_DYY = <double*> malloc(width * height * sizeof(double))
    input_grayscale_DXY = <double*> malloc(width * height * sizeof(double))

    for j in range(height):
        for i in range(width):
            double_input_grayscale[j*width + i] = np.float64(imagearray[j, i])

    GaussianDerivativeX(double_input_grayscale, width, height, input_grayscale_DX, np.float64(sigma))
    GaussianDerivativeY(double_input_grayscale, width, height, input_grayscale_DY, np.float64(sigma))
    GaussianDerivativeXX(double_input_grayscale, width, height, input_grayscale_DXX, np.float64(sigma))
    GaussianDerivativeYY(double_input_grayscale, width, height, input_grayscale_DYY, np.float64(sigma))
    GaussianDerivativeXY(double_input_grayscale, width, height, input_grayscale_DXY, np.float64(sigma))

    image_dx_array = np.ndarray((height, width), dtype=np.float64)
    for i in range(width):
        for j in range(height):
            pixel_address = j*width + i
            pixel_intensity = input_grayscale_DX[pixel_address]
            image_dx_array[j, i] = np.float64(pixel_intensity)

    image_dy_array = np.ndarray((height, width), dtype=np.float64)
    for i in range(width):
        for j in range(height):
            pixel_address = j*width + i
            pixel_intensity = input_grayscale_DY[pixel_address]
            image_dy_array[j, i] = np.float64(pixel_intensity)

    image_dxx_array = np.ndarray((height, width), dtype=np.float64)
    for i in range(width):
        for j in range(height):
            pixel_address = j*width + i
            pixel_intensity = input_grayscale_DXX[pixel_address]
            image_dxx_array[j, i] = np.float64(pixel_intensity)

    image_dyy_array = np.ndarray((height, width), dtype=np.float64)
    for i in range(width):
        for j in range(height):
            pixel_address = j*width + i
            pixel_intensity = input_grayscale_DYY[pixel_address]
            image_dyy_array[j, i] = np.float64(pixel_intensity)

    image_dxy_array = np.ndarray((height, width), dtype=np.float64)
    for i in range(width):
        for j in range(height):
            pixel_address = j*width + i
            pixel_intensity = input_grayscale_DXY[pixel_address]
            image_dxy_array[j, i] = np.float64(pixel_intensity)

    free(double_input_grayscale)
    free(input_grayscale_DX)
    free(input_grayscale_DY)
    free(input_grayscale_DXX)
    free(input_grayscale_DYY)
    free(input_grayscale_DXY)

    return np.stack((image_dx_array, image_dy_array, image_dxx_array, image_dyy_array, image_dxy_array), axis=2)


cpdef get_image_edges(imagearray, sigma=2, upper_threshold=10.0, lower_threshold=5.0):
    cdef unsigned char* input_grayscale
    cdef unsigned char* temp_edge_image
    cdef int width
    cdef int height

    height, width = imagearray.shape
    cdef int pitch = int(math.ceil(width/4.0)*4.0)
    input_grayscale = <unsigned char*> malloc(pitch*height*sizeof(unsigned char))
    temp_edge_image = <unsigned char*> malloc(pitch*height*sizeof(unsigned char))

    for j in range(height):
        for i in range(width):
            input_grayscale[pitch * j + i] = imagearray[j, i]

    CannyEdgeDetector(
        input_grayscale, width, height, temp_edge_image,
        np.float64(sigma), np.float64(upper_threshold), np.float64(lower_threshold))

    edges = np.zeros((height, width), dtype=np.uint8)
    for j in range(height):
        for i in range(width):
            edges[j, i] = temp_edge_image[pitch*j + i]

    free(input_grayscale)
    free(temp_edge_image)

    return edges


cpdef get_image_corners(imagearray, sigma=2.0, sigma_w=2.0, k=0.06, threshold=1000.0):
    cdef unsigned char* input_grayscale
    cdef unsigned char* temp_corner_image
    cdef int width
    cdef int height

    height, width = imagearray.shape
    cdef int pitch = int(math.ceil(width/4.0)*4.0)
    input_grayscale = <unsigned char*> malloc(pitch*height*sizeof(unsigned char))
    temp_corner_image = <unsigned char*> malloc(pitch*height*sizeof(unsigned char))

    for j in range(height):
        for i in range(width):
            input_grayscale[pitch * j + i] = imagearray[j, i]

    CornerDetector(
        input_grayscale, width, height, temp_corner_image, np.float64(sigma), np.float64(sigma_w), np.float64(k), np.float64(threshold))

    edges = np.zeros((height, width), dtype=np.uint8)
    for j in range(height):
        for i in range(width):
            edges[j, i] = temp_corner_image[pitch*j + i]

    free(input_grayscale)
    free(temp_corner_image)

    return edges


cpdef get_gaussian_kernel1d(sigma, kernelsize):
    cdef double* kernel

    kernel = <double*> malloc(kernelsize * sizeof(double));
    getGaussianKernel1D(kernel, np.float64(sigma), int(kernelsize))

    kernel_array = np.ndarray((kernelsize,), dtype=np.float64)
    for i in range(kernelsize):
        kernel_array[i] = np.float64(kernel[i])

    free(kernel)

    return kernel_array


cpdef get_dgaussian_kernel1d(sigma, kernelsize):
    cdef double* kernel

    kernel = <double*> malloc(kernelsize * sizeof(double));
    getDGausianKernel1D(kernel, np.float64(sigma), int(kernelsize))

    kernel_array = np.ndarray((kernelsize,), dtype=np.float64)
    for i in range(kernelsize):
        kernel_array[i] = np.float64(kernel[i])

    free(kernel)

    return kernel_array


cpdef get_d2gaussian_kernel1d(sigma, kernelsize):
    cdef double* kernel

    kernel = <double*> malloc(kernelsize * sizeof(double));
    getD2GausianKernel1D(kernel, np.float64(sigma), int(kernelsize))

    kernel_array = np.ndarray((kernelsize,), dtype=np.float64)
    for i in range(kernelsize):
        kernel_array[i] = np.float64(kernel[i])

    free(kernel)

    return kernel_array
