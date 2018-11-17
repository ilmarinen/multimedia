# Linux Multimedia

## Build

1. `mkdir build`
2. `gcc src/multimedia.c build/multimedia` or `gcc -g3 src/multimedia.c -o build/multimedia` (so that you can use GDB and Valgrind).

## Usage

### Standard Capture

`build/multimedia -c <number-of-frames-to-capture> -o <output-file-string>`

### Standard Capture with Cropping

`build/multimedia -c <number-of-frames-to-capture> -w x1,y1,x2,y2 -o <output-file-string>`
