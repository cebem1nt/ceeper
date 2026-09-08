#include "common.hpp"

#include <random>

std::vector<uchar> urandom(uint nbytes) 
{
    // Might not be very good
    std::vector<uchar> out(nbytes);
    std::random_device random;

    for (unsigned int i = 0; i < nbytes; i++) {
        out[i] = SC<uchar>(random());
    }

    return out;
}

std::string surandom(uint nbytes)
{
    auto randbytes = urandom(nbytes);
    return std::string(randbytes.begin(), randbytes.end());
}

