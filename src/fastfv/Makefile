# Will link statically with OpenCV, Yael, OpenMP, OpenBLAS, gfortran. To link dynamically, remove "-Wl,-Bstatic" and "-Wl,-Bdynamic"

CFLAGS = -O3 -msse3 -std=c++0x
LDFLAGS = -lopencv_flann -lopencv_core -Wl,-Bstatic -lyael -lgomp -lopenblas -lgfortran -Wl,-Bdynamic -lpthread -lz -lm -lc -lrt
INSTALLED_DEPS = -I../../bin/3rdparty/include -L../../bin/3rdparty/lib 
BIN = ../../bin/fastfv

all: $(SOURCE_FILES)
	g++ main.cpp -o $(BIN) $(CFLAGS) $(LDFLAGS) $(INSTALLED_DEPS)
clean:
	rm $(BIN)
