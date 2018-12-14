from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize
import numpy

ext_modules = [
    Extension("pymultimedia",
              sources=["src/pymultimedia.pyx", "src/camera.c", "src/imageprocessing.c"])
]

setup(name="PyMultimedia",
      include_dirs=[numpy.get_include()],
      ext_modules=cythonize(ext_modules))
