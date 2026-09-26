CXX      = clang++
CXXFLAGS = -Wall -Wextra -std=c++23
LFLAGS	 = -lcrypto

SRCS     = $(wildcard src/*.cpp)
OBJS     = $(SRCS:src/%.cpp=build/%.o)

ifneq ($(OS),Windows_NT)
	UNAME_S := $(shell uname -s)
	LFLAGS += -lreadline

	ifeq ($(UNAME_S),Darwin)
		OBJS += build/clipboard.o
		LFLAGS += -framework AppKit
	endif
endif

.PHONY: all run clean build/prepare

all: build/cee

build/prepare:
	mkdir -p build

build/clipboard.o: include/clipboard.mm | build/prepare
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: src/%.cpp | build/prepare
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/cee: $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $@

run: build/cee
	./build/cee

install: build/cee
	cp build/cee /usr/bin/

uninstall: build/cee
	rm /usr/bin/cee

clean:
	rm -rf build/