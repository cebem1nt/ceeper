#include "common.hpp"

#include <random>
#include <algorithm>

std::string to_lower(std::string in) 
{
    std::transform(in.begin(), in.end(), in.data(), ::tolower);
    return in;
}

std::vector<uchar> urandom(uint nbytes) 
{
    // Might not be very good
    std::vector<uchar> out(nbytes);

    std::random_device random;
    std::mt19937_64 gen(random());
    std::uniform_int_distribution<int> dist(0, 255);

    for (unsigned int i = 0; i < nbytes; i++) {
        out[i] = SC<uchar>(dist(gen));
    }

    return out;
}

std::string surandom(uint nbytes)
{
    auto randbytes = urandom(nbytes);
    return std::string(randbytes.begin(), randbytes.end());
}
