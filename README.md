# Linux Multimedia

## Environment

1. `python3 -m venv /path/to/new/virtual/environment`
2. `source /path/to/new/virtual/environment/bin/activate`
3. `pip install cython`

## Build

1. `mkdir build && mkdir include`
2. `make`

## Tests
1. `gcc src/test_imageprocessing.c src/imageprocessing.c build/test_imageprocessing`
2. `./build/test_imageprocessing`

## Usage

### Standard Capture

`build/multimedia -c <number-of-frames-to-capture> -o <output-file-string>`

### Standard Capture with Cropping

`build/multimedia -c <number-of-frames-to-capture> -w x1,y1,x2,y2 -o <output-file-string>`

## Notes

### Make

A makefile typically has a start where variables are diclared at the start, eg:

```
CC = gcc
LIB_DIR=lib
INCLUDE_DIR=include
SRC_DIR=src
BUILD_DIR=build
```

These variables may be referred to later by `$(variable-name)`, in which case the
reference to the variable is replaced by the expanded value of the variable.
E.g.: `$(CC)` gets expanded to `gcc`

The remaining parts of the makefile are of the form:

```
<target-name>: <dependency-1> <dependency-2>
    <action-for-building-target>
```

A call to make of the form `make <target-name>` will check for the presence and
timestamp of the target. It will also check the presence and timestamps of the
dependencies. If they don't exist it throws an error. If present and their
timestamps indicate that they are more up to date than the target or if the target
does not exist, then the action for building the target is performed.

For example:

```
default: $(BUILD_DIR)/multimedia pymultimedia

pymultimedia: setup.py $(SRC_DIR)/pymultimedia.pyx $(SRC_DIR)/camera.c $(SRC_DIR)/imageprocessing.c
    python3 setup.py build_ext --inplace && rm -f $(SRC_DIR)/pymultimedia.c

$(BUILD_DIR)/multimedia: $(SRC_DIR)/multimedia.c $(SRC_DIR)/camera.c $(SRC_DIR)/imageprocessing.c
    $(CC) -g3 $^ -o $@

clean:
    rm -f *.o *.a *.so $(BUILD_DIR)/multimedia
```

The above script will attempt to build the targets `multimedia` and `pymultimedia` by running
`python3 setup.py build_ext --inplace && rm -f src/pymultimedia.c` and
`$(CC) -g3 src/multimedia.c src/camera.c src/imageprocessing.c -o build/multimedia`
at the command line.

It will only attempt to build those targets if their respective dependencies have fresher
timestamps. If for some reasona a dependency is missing, then make will throw an error.

### Cython

Cython is a compiled language that is a superset of Python, with the addition of
C like syntax. This amounts to Python type free syntax, together with C like
typed syntax which compiles to more efficient runtime code. Cython also allows
for the possibility of calling out to C or C++ modules and functionality.

A Cython module gets compiled to C, which then gets compiled to a dynamic library
file, i.e. an `*.so` file on Linux. This library can then be loaded from within
Python by simply importing the module by name, and its functionality can then
be accessed from within Python.

All the information needed for compiling the dynamic library `so` file is specified
within the modules `setup.py` file.

