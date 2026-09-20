#pragma once

#include <format>
#include <string>
#include <vector>

#if defined (__ANDROID__)
#   define PLATFORM "Android"
#elif defined (_WIN32)
#   define PLATFORM "Windows"
#elif defined (__APPLE__) || defined (__MACH__)
#   define PLATFORM "Darwin"
#elif defined(__linux__)
#   define PLATFORM "Posix"
#else
#    error Unsupported platform
#endif

#ifdef __linux__
#   include <readline/readline.h>
#   include <readline/history.h>
#else
#   include <iostream>
#endif

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
std::string trim_whitespace(std::string in);

std::vector<std::string> splitstr(const std::string& in, const char delim = ' ');

void toggle_stdin_echo(bool enable = true);

template <class... Args>
std::string input(std::format_string<Args...> prompt, Args&&... args)
{
    auto formatted = std::format(prompt, std::forward<Args>(args)...);
    std::string out;

#ifdef __linux__
    char* line = readline(formatted.c_str()); 

    if (!line)
        return {};

    add_history(line);
    out = std::string(line);
    free(line);
#else
    std::cout << formatted;
    std::getline(std::cin, out);
#endif

    return out;
}

template <class... Args>
std::string getpasswd(std::format_string<Args...> prompt, Args&&... args)
{
    toggle_stdin_echo(false);
    auto out = input(prompt, std::forward<Args>(args)...);
    toggle_stdin_echo(true);

    return out;
}

std::string input();
std::string getpasswd();

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