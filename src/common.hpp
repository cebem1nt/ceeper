#pragma once

#include <string>

#define FATAL(...) do {                 \
    std::fprintf(stderr, __VA_ARGS__);  \
    exit(EXIT_FAILURE);                 \
} while (0);

#define RC reinterpret_cast
#define SC static_cast

typedef unsigned char uchar;
typedef unsigned int  uint;

std::string urandom(uint nbytes);