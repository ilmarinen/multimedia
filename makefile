CC = gcc
SRC_DIR=src
BUILD_DIR=build

default: $(BUILD_DIR)/multimedia pymultimedia

test: $(BUILD_DIR)/test_imageprocessing

$(BUILD_DIR)/test_imageprocessing: $(SRC_DIR)/test_imageprocessing.c $(SRC_DIR)/camera.c $(SRC_DIR)/imageprocessing.c
	$(CC) -g3 $^ -o $@ && $(BUILD_DIR)/test_imageprocessing


pymultimedia: setup.py $(SRC_DIR)/pymultimedia.pyx $(SRC_DIR)/camera.c $(SRC_DIR)/imageprocessing.c
	python3 setup.py build_ext --inplace && rm -f $(SRC_DIR)/pymultimedia.c

$(BUILD_DIR)/multimedia: $(SRC_DIR)/multimedia.c $(SRC_DIR)/camera.c $(SRC_DIR)/imageprocessing.c
	$(CC) -g3 $^ -o $@

clean:
	rm -f *.o *.a *.so $(BUILD_DIR)/multimedia
