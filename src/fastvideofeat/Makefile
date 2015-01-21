# Will link statically with OpenCV, FFmpeg

CFLAGS = -O3 -D__STDC_CONSTANT_MACROS
LDFLAGS = -lopencv_imgproc -lopencv_core -lswscale -lavdevice -lavformat -lavcodec -lswresample -lavutil -lpthread -lbz2 -lz -lc -lrt
INSTALLED_DEPS = -I../../bin/3rdparty/include -L../../bin/3rdparty/lib 
BIN = ../../bin/fastvideofeat

all:
	g++ main.cpp -o $(BIN) $(CFLAGS) $(LDFLAGS) $(INSTALLED_DEPS)
clean:
	rm $(BIN)
