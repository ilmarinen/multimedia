import os
from PIL import Image

import pymultimedia

def test_grab_frame_rgb():
    dev_handle = pymultimedia.py_open_device("/dev/video0")
    width, height = pymultimedia.py_get_resolution(dev_handle)
    pymultimedia.py_close_device(dev_handle)
    image_ndarray = pymultimedia.py_grab_frame_rgb("/dev/video0", width, height)
    assert(image_ndarray.shape == (height, width, 3))
    img = Image.fromarray(image_ndarray, "RGB")
    img.save("test.bmp")

    new_img = Image.open("test.bmp")
    assert(new_img.size == (width, height))

    os.remove("test.bmp")

def test_grab_frame_grayscale():
    dev_handle = pymultimedia.py_open_device("/dev/video0")
    width, height = pymultimedia.py_get_resolution(dev_handle)
    pymultimedia.py_close_device(dev_handle)
    image_ndarray = pymultimedia.py_grab_frame_grayscale("/dev/video0", width, height)
    assert(image_ndarray.shape == (height, width))
    img = Image.fromarray(image_ndarray, "L")
    img.save("test.bmp")

    new_img = Image.open("test.bmp")
    assert(new_img.size == (width, height))

    os.remove("test.bmp")