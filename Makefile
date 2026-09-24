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

build/frontend.o: include/argparse.hpp
build/frontend.o: CXXFLAGS += -include include/argparse.hpp

build/%.o: src/%.cpp | build/prepare
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/cee: $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $@

run: build/cee
	./build/cee

clean:
	rm -rf build/