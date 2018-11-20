# Linux Multimedia

## Environment

1. `python3 -m venv /path/to/new/virtual/environment`
2. `source /path/to/new/virtual/environment/bin/activate`
3. `pip install cython`

## Build

1. `mkdir build`
2. `gcc src/multimedia.c src/imageprocessing.c src/camera.c -o build/multimedia -lm` or `gcc -g3 src/multimedia.c src/imageprocessing.c src/camera.c -o build/multimedia -lm` (so that you can use GDB and Valgrind).

## Tests
1. `gcc src/test_imageprocessing.c src/imageprocessing.c build/test_imageprocessing`
2. `./build/test_imageprocessing`

## Usage

### Standard Capture

`build/multimedia -c <number-of-frames-to-capture> -o <output-file-string>`

### Standard Capture with Cropping

`build/multimedia -c <number-of-frames-to-capture> -w x1,y1,x2,y2 -o <output-file-string>`
