CFLAGS = -D__STDC_CONSTANT_MACROS -O3 -rdynamic
SOURCE_FILES = main.cpp
LIBS = -lc -lopencv_core -lopencv_imgproc -lavcodec -lavformat -lavutil -lswscale  

all: $(SOURCE_FILES)
	mkdir -p build
	$(CC) $(SOURCE_FILES) -o build/fastvideofeat $(CFLAGS) $(LIBS)

clean:
	rm -rf build
