PREFIX ?= /usr/local
PYTHON ?= python3
INCLUDES=`$(PYTHON) -m pybind11 --includes`
LIBS=-lblkid `$(PYTHON)-config --embed --libs`
ARCH ?= $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/' | sed 's/riscv64/riscv/')

ifdef WITH_EXEC_GUARD
EXTRA_SRCS = exec_guard.cpp
EXTRA_LIBS = -lbpf
CXXFLAGS += -DWITH_EXEC_GUARD
PREREQS = exec_guard.skel.h
endif

MAIN_SRCS = $(filter-out exec_guard.cpp, $(wildcard *.cpp)) $(wildcard native/*.cpp)
ALL_SRCS = $(MAIN_SRCS) $(EXTRA_SRCS)

DEBUG_SRCS = $(filter-out exec_guard.cpp, $(wildcard *.cpp))
DEBUG_OBJS=$(filter-out debug/genpack-init.o,$(patsubst %.cpp,debug/%.o,$(DEBUG_SRCS))) \
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

tests: $(patsubst %.cpp,debug/%.bin,$(DEBUG_SRCS))

debug/%.bin: %.cpp $(DEBUG_OBJS) $(wildcard *.h) $(wildcard native/*.h)
	g++ -std=c++23 -g -o $@ $< -DTEST $(INCLUDES) $(LIBS) $(filter-out $(patsubst debug/%.bin,debug/%.o,$@),$(DEBUG_OBJS))

VMLINUX_BTF := $(firstword $(wildcard /usr/src/linux/vmlinux) /sys/kernel/btf/vmlinux)

vmlinux.h:
	bpftool btf dump file $(VMLINUX_BTF) format c > $@

exec_guard.bpf.o: exec_guard.bpf.c vmlinux.h
	clang -O2 -g -target bpf -D__TARGET_ARCH_$(ARCH) -c -o $@ $<

exec_guard.skel.h: exec_guard.bpf.o
	bpftool gen skeleton $< > $@

genpack-init: $(ALL_SRCS) $(wildcard *.h) $(wildcard native/*.h) $(PREREQS)
	g++ -std=c++23 -o $@ $(ALL_SRCS) $(INCLUDES) $(LIBS) $(EXTRA_LIBS) $(CXXFLAGS) -static-libgcc -static-libstdc++

clean:
	rm -rf debug genpack-init vmlinux.h exec_guard.bpf.o exec_guard.skel.h

install: genpack-init
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 genpack-init $(DESTDIR)$(PREFIX)/bin/genpack-init
