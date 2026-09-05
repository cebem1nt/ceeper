CXX      = clang++
CXXFLAGS = -Wall -Wextra -std=c++20
SRCS     = src/*.cpp

build: $(SRCS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRCS) -o build/ceeper

run: build
	./build/ceeper

clean:
	rm -rf build/