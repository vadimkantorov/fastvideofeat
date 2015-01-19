CFLAGS = -O3 -rdynamic -msse3 -std=c++0x
LDFLAGS = -lopencv_core -lopencv_flann -lyael 
INSTALLED_DEPS = -I../../3rdparty/include -L../../3rdparty/lib 

all: $(SOURCE_FILES)
	mkdir -p build
	g++ main.cpp -o build/fastfv $(CFLAGS) $(LDFLAGS) $(INSTALLED_DEPS)

clean:
	rm -rf build
