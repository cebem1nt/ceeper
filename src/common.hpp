#pragma once

#define FATAL(...) do {                 \
    std::fprintf(stderr, __VA_ARGS__);  \
    exit(EXIT_FAILURE);                 \
} while (0);
