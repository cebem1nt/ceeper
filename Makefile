CXX           = clang++
CXXFLAGS      = -Wall -Wextra -std=c++23
LFLAGS	      = -lcrypto

SRCS          = $(wildcard src/*.cpp)
OBJS          = $(SRCS:src/%.cpp=build/%.o)

ARGPARSE 	  = build/argparse.hpp.pch
ARGPARSE_SRC  = src/argparse.hpp

.PHONY: all run clean build/prepare

all: build/ceeper

build/prepare:
	mkdir -p build

$(ARGPARSE): $(ARGPARSE_SRC) | build/prepare
	$(CXX) $(CXXFLAGS) -x c++-header $< -o $@

build/%.o: src/%.cpp $(ARGPARSE) | build/prepare
	$(CXX) $(CXXFLAGS) -include $(ARGPARSE_SRC) -c $< -o $@

build/ceeper: build/prepare $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $@

run: build/ceeper
	./build/ceeper

clean:
	rm -rf build/