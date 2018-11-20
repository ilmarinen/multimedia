# Linux Multimedia

## Build

1. `mkdir build`
2. `gcc src/multimedia.c src/imageprocessing.c build/multimedia` or `gcc -g3 src/multimedia.c src/imageprocessing.c -o build/multimedia` (so that you can use GDB and Valgrind).

## Tests
1. `gcc src/test_imageprocessing.c src/imageprocessing.c build/test_imageprocessing`
2. `./build/test_imageprocessing`

## Usage

### Standard Capture

`build/multimedia -c <number-of-frames-to-capture> -o <output-file-string>`

### Standard Capture with Cropping

`build/multimedia -c <number-of-frames-to-capture> -w x1,y1,x2,y2 -o <output-file-string>`
