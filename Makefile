PREFIX ?= /usr/local
PYTHON ?= python3

all: genpack-init.bin coldplug.bin

genpack-init.bin: genpack-init.cpp coldplug.cpp
	g++ -o $@ $^ $(CXXFLAGS) `$(PYTHON) -m pybind11 --includes` `$(PYTHON)-config --embed --libs` -static-libgcc -static-libstdc++

coldplug.bin: coldplug.cpp
	g++ -o $@ $^ $(CXXFLAGS) -DTEST_COLDPLUG `$(PYTHON) -m pybind11 --includes` `$(PYTHON)-config --embed --libs`

clean:
	rm -f *.bin

install: genpack-init.bin
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 genpack-init.bin $(DESTDIR)$(PREFIX)/bin/genpack-init
