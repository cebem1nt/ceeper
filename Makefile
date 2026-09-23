# TODO build aint crossplatform :(

CXX           = clang++
CXXFLAGS      = -Wall -Wextra -std=c++23
LFLAGS	      = -lcrypto -lreadline

SRCS          = $(wildcard src/*.cpp)
OBJS          = $(SRCS:src/%.cpp=build/%.o)

ARGPARSE 	  = build/argparse.hpp.pch
ARGPARSE_SRC  = include/argparse.hpp

.PHONY: all run clean build/prepare

all: build/cee

build/prepare:
	mkdir -p build

$(ARGPARSE): $(ARGPARSE_SRC) | build/prepare
	$(CXX) $(CXXFLAGS) -x c++-header $< -o $@

build/%.o: src/%.cpp $(ARGPARSE) | build/prepare
	$(CXX) $(CXXFLAGS) -include $(ARGPARSE_SRC) -c $< -o $@

build/cee: build/prepare $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $@

run: build/cee
	./build/cee

clean:
	rm -rf build/