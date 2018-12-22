# Linux Multimedia

## Environment

1. `python3 -m venv /path/to/new/virtual/environment`
2. `source /path/to/new/virtual/environment/bin/activate`
3. `sudo apt-get install libpython3.6-dev python3-dbg`
4. `pip install -r requirements.txt`

## Build

1. `mkdir build && mkdir include`
2. `make`

## Tests
1. `make test`

## Usage

### Standard Capture

`build/multimedia -c <number-of-frames-to-capture> -o <output-file-string>`

### Standard Capture with Cropping

`build/multimedia -c <number-of-frames-to-capture> -w x1,y1,x2,y2 -o <output-file-string>`

## Notes

### Make

A makefile typically has a start where variables are declared at the start, eg:

```
CC=gcc
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
timestamps. If for some reason a dependency is missing, then make will throw an error.

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

### Building C / C++ extensions for Python

An excellent resource on this is [here](https://docs.python.org/2/extending/building.html)
The TLDR is as follows:

If the following script is in `setup.py`


```
from distutils.core import setup, Extension

module1 = Extension('demo',
                    define_macros = [('MAJOR_VERSION', '1'),
                                     ('MINOR_VERSION', '0')],
                    include_dirs = ['/usr/local/include'],
                    libraries = ['tcl83'],
                    library_dirs = ['/usr/local/lib'],
                    sources = ['demo.c'])

setup (name = 'PackageName',
       version = '1.0',
       description = 'This is a demo package',
       author = 'Martin v. Loewis',
       author_email = 'martin@v.loewis.de',
       url = 'https://docs.python.org/extending/building',
       ext_modules = [module1])
```

Then running `python setup.py build_ext --inplace` will result in distutils calling out to
the compiler on Linux as follows:

```
gcc -DNDEBUG -g -O3 -Wall -Wstrict-prototypes -fPIC -DMAJOR_VERSION=1 -DMINOR_VERSION=0 -I/usr/local/include -I/usr/local/include/python2.2 -c demo.c -o build/temp.linux-i686-2.2/demo.o

gcc -shared build/temp.linux-i686-2.2/demo.o -L/usr/local/lib -ltcl83 -o build/lib.linux-i686-2.2/demo.so
```

### Using GDB

Gdb is excellent for debugging. Using it with C code is relatively straightforward.

1. Compile the C code with the `-g3` argument to make sure that debugging symbols are
included.
2. Set any breakpoints you need. For example `break code_filename.c:162` to set a breakpoint
at line 162 of `code_filename.c`.
3. Then run the program with `r <executable-name> <arguments>`

You can also use gdb to debug Python code. This is especially useful for debugging
issues with the Pymultimedia Cython extension. In order to do this you need to have
the Python debugging symbols installed. To do this for Python3 simply do:

`sudo apt-get install python3-dbg`

Once those are installed you can debug as follows:

1. `gdb python`
2. Set any breakpoints you need, even breakpoints in C files.
3. `r <program-name>.py <arguments>`

### Development Guidelines

If you are interesting in contributing to this project, that's excellent and welcomed! There are some guidelines for managing branches and pull requests.

#### How to Manage a Pull Request
1. Before making the pull request, rebase off master and squash all commits in the branch into one commit.
- *If you are unfamiliar with rebasing, please ask other contributor and find some resources, like this one*:
-- https://git-scm.com/book/en/v2/Git-Branching-Rebasing
2. Commit messages should have one short summary line on the order of 65 chars. Then a blank line and a paragraph description of the commit using assertive tense, i.e.  implement the edge detector and rename function A to function B etc.
3. During code review if changes are requested, push those as individual commits so that the reviewer(s) can see the changes being made.
4. Code review approval is with a mention and a comment of +1 or +2. The +1 means, "I don't really understand all the code but this looks right to me". The +2 means, "I have read all the code and understand it and this looks right to me". In order to merge code, you need a +2 or two +1s from different reviewers.
