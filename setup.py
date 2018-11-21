from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

ext_modules = [
    Extension("pymultimedia",
              sources=["src/pymultimedia.pyx", "src/camera.c", "src/imageprocessing.c"])
]

setup(name="PyMultimedia",
      ext_modules=cythonize(ext_modules))
