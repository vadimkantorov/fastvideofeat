# Will link statically with OpenCV, Yael, ATLAS, gfortran. To link dynamically, remove "-Wl,-Bstatic" and "-Wl,-Bdynamic"

CFLAGS = -O3 -msse3 -std=c++0x
LDFLAGS = -lopencv_flann -lopencv_core -Wl,-Bstatic -lyael -lgomp -llapack -lf77blas -lcblas -latlas -lgfortran -Wl,-Bdynamic -lpthread -lz -lm -lc -lrt
INSTALLED_DEPS = -I../../bin/dependencies/include -L../../bin/dependencies/lib -I../../bin/dependencies/yael -L../../bin/dependencies/yael/yael
BIN = ../../bin/fastfv

all: $(SOURCE_FILES)
	g++ main.cpp -o $(BIN) $(CFLAGS) $(LDFLAGS) $(INSTALLED_DEPS)
clean:
	rm $(BIN)
