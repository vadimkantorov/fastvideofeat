SOURCE_FILES = main.cpp
CFLAGS = -O3 -rdynamic -msse3 -std=c++0x
LDFLAGS =	-lopencv_core \
		-lopencv_flann \
		-lyael \
		-lhdf5_hl \
		-lhdf5

BUILD_DIR = build
CONFIG_FILE = $(BUILD_DIR)/config.h

all: $(SOURCE_FILES)
	mkdir -p $(BUILD_DIR)
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
	$(CXX) $(SOURCE_FILES) -I$(BUILD_DIR) -o $(BUILD_DIR)/fastfv $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)
