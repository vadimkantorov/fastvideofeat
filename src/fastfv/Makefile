CFLAGS = -O3 -rdynamic -msse3 -std=c++0x

LIBS =	-lopencv_core \
		-lopencv_flann \
		-lyael \
		-lhdf5_hl \
		-lhdf5 \

SOURCE_FILES = main.cpp

BUILD_DIR = build
CONFIG_FILE = $(BUILD_DIR)/config.h

all: $(SOURCE_FILES)
	mkdir -p build
	echo > $(CONFIG_FILE)
ifneq ($(WITH_HDF5),OFF)
	echo '#define HAVE_HDF5 1' >> $(CONFIG_FILE)
else
	echo '#define HAVE_HDF5 0' >> $(CONFIG_FILE)
endif

ifneq ($(WITH_YAEL),OFF)
	echo '#define HAVE_YAEL 1' >> $(CONFIG_FILE)
else
	echo '#define HAVE_YAEL 0' >> $(CONFIG_FILE)
endif
	$(CC) $(SOURCE_FILES) $(LIBS) $(CFLAGS) -I$(BUILD_DIR) -o $(BUILD_DIR)/fv_fast

clean:
	rm -rf build
