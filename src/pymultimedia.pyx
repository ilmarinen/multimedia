cdef extern from "camera.h":
    cdef int open_device(char* device_name)
    cdef void close_device(int device_handle)


cpdef int py_open_device(dev_name):
    cdef bytes dev_name_bytes = dev_name.encode()
    cdef char* d_name = dev_name_bytes

    return open_device(d_name)


cpdef py_close_device(int dev_handle):
    close_device(dev_handle)
