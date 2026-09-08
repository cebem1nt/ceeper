CXX      = clang++
CXXFLAGS = -Wall -Wextra -std=c++20
LFLAGS	 = -lcrypto
SRCS     = src/*.cpp

build: $(SRCS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(SRCS) -o build/ceeper

run: build
	./build/ceeper

clean:
	rm -rf build/