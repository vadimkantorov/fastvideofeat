SOURCE_FILES = main.cpp
CFLAGS = -D__STDC_CONSTANT_MACROS -O3 -rdynamic
LDFLAGS = -lc -lopencv_core -lopencv_imgproc -lavcodec -lavformat -lavutil -lswscale -lswresample
INSTALLED_DEPS = -I../../3rdparty/include -L../../3rdparty/lib 

all: $(SOURCE_FILES)
	mkdir -p build
	$(CXX) $(SOURCE_FILES) -o build/fastvideofeat $(CFLAGS) $(LDFLAGS) $(INSTALLED_DEPS)

clean:
	rm -rf build
