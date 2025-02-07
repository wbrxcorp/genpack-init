PREFIX ?= /usr/local
PYTHON ?= python3
INCLUDES=`$(PYTHON) -m pybind11 --includes`
LIBS=-lblkid `$(PYTHON)-config --embed --libs`
DEBUG_OBJS=$(filter-out debug/genpack-init.o,$(patsubst %.cpp,debug/%.o,$(wildcard *.cpp))) \
	$(patsubst native/%.cpp,debug/native/%.o,$(wildcard native/*.cpp))

.PHONY: all tests clean install

all: genpack-init

debug:
	mkdir -p debug

debug/native:
	mkdir -p debug/native

debug/%.o: %.cpp $(wildcard *.h) $(wildcard native/*.h) | debug
	g++ -std=c++23 -g -c -o $@ $< $(INCLUDES) -DDEBUG

debug/native/%.o: native/%.cpp $(wildcard native/*.h) | debug/native
	g++ -std=c++23 -g -c -o $@ $< $(INCLUDES) -DDEBUG

tests: $(patsubst %.cpp,debug/%.bin,$(wildcard *.cpp))

debug/%.bin: %.cpp $(DEBUG_OBJS) $(wildcard *.h) $(wildcard native/*.h)
	g++ -std=c++23 -g -o $@ $< -DTEST $(INCLUDES) $(LIBS) $(filter-out $(patsubst debug/%.bin,debug/%.o,$@),$(DEBUG_OBJS))

genpack-init: $(wildcard *.cpp) $(wildcard native/*.cpp) $(wildcard *.h) $(wildcard native/*.h)
	g++ -std=c++23 -o $@ $(wildcard *.cpp) $(wildcard native/*.cpp) $(INCLUDES) $(LIBS) -static-libgcc -static-libstdc++

clean:
	rm -rf debug genpack-init

install: genpack-init
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 genpack-init $(DESTDIR)$(PREFIX)/bin/genpack-init
