#pragma once

#include <string>
#include <vector>

#define FATAL(...) do {                 \
    std::fprintf(stderr, __VA_ARGS__);  \
    exit(EXIT_FAILURE);                 \
} while (0);

#define RC reinterpret_cast
#define SC static_cast

typedef unsigned char uchar;
typedef unsigned int  uint;

std::vector<uchar> urandom(uint nbytes);
std::string surandom(uint nbytes);

std::string to_lower(std::string in);

namespace ceeper {
     // 0 - Tag, 1 - Login, 2 - Password
    using Triplet = std::array<std::string, 3>;
}

namespace exc {
    class Exception
        : public std::exception 
    {
    public:
        Exception(std::string msg):
            msg_(msg) {};

        const char* what() const noexcept {
            return msg_.c_str();
        }
    protected:
        std::string msg_;
    };

    class FileNotFound   : public Exception { using Exception::Exception; };
    class AsertionFailed : public Exception { using Exception::Exception; };
    class ValueMismatch  : public Exception { using Exception::Exception; };
    class NotInitialized : public Exception { using Exception::Exception; };
    class FileMalformed  : public Exception { using Exception::Exception; };
    class EncryptionError: public Exception { using Exception::Exception; };
    class DecryptionError: public Exception { using Exception::Exception; };
    class AlreadyExists  : public Exception { using Exception::Exception; };
    class NotFound       : public Exception { using Exception::Exception; };

} // namespace exc;