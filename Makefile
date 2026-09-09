CXX      = clang++
CXXFLAGS = -Wall -Wextra -std=c++23
LFLAGS	 = -lcrypto

SRCS     = $(wildcard src/*.cpp)
OBJS     = $(SRCS:src/%.cpp=build/%.o)

all: build/ceeper

build:
	mkdir -p build

build/ceeper: build $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: build
	./build/ceeper

clean:
	rm -rf build/