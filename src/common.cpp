#include "common.hpp"

#include <random>

std::string urandom(unsigned int nbytes) 
{
    std::random_device random;
    auto buff = std::string(nbytes, '\0');

    for (unsigned int i = 0; i < nbytes; i++) {
        buff[i] = static_cast<char>(random());
    }

    return buff;
}
