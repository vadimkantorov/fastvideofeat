SOURCE_FILES = main.cpp
CFLAGS = -O3 -rdynamic -msse3 -std=c++0x
LDFLAGS =	-lopencv_core \
		-lopencv_flann \
		-lyael \
		-lhdf5_hl \
		-lhdf5

BUILD_DIR = build

INSTALLED_DEPS = -I../../3rdparty/include -L../../3rdparty/lib 

all: $(SOURCE_FILES)
	mkdir -p $(BUILD_DIR)

	$(CXX) $(SOURCE_FILES) -I$(BUILD_DIR) -o $(BUILD_DIR)/fastfv $(CFLAGS) $(LDFLAGS) $(INSTALLED_DEPS)

clean:
	rm -rf $(BUILD_DIR)
