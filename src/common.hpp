#pragma once

#include <format>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>

#if defined (_WIN32)
# include <windows.h>
#endif

#ifdef __linux__
# include <termios.h>
# include <unistd.h>
# include <readline/readline.h>
# include <readline/history.h>
#endif

#define RC reinterpret_cast
#define SC static_cast

typedef unsigned char uchar;
typedef unsigned int  uint;

typedef std::array<std::string, 3> Triplet;

std::vector<uchar> urandom(uint nbytes);
std::string surandom(uint nbytes);

std::string to_lower(std::string in);
std::string trim_whitespace(std::string in);

std::vector<std::string> splitstr(const std::string& in, const char delim = ' ');
void clear_screen();

std::optional<std::string> input();
std::optional<std::string> getpasswd();

std::string readfile(std::filesystem::path file);

// https://sqlpey.com/c++/cpp-cross-platform-stdin-echo-control/
void toggle_stdin_echo(bool enable = true);

namespace exc 
{
    class Exception
        : public std::exception 
    {
    public:
        Exception(std::string msg):
            msg_(msg) {}

        template<class... Args>
        Exception(std::format_string<Args...> msg, Args&&... args):
            msg_(std::format(msg, std::forward<Args>(args)...)) {}
        
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
    class AlreadyExists  : public Exception { using Exception::Exception; };
    class NotFound       : public Exception { using Exception::Exception; };
    class NotImplemented : public Exception { using Exception::Exception; };
    class EncryptionError: public Exception { using Exception::Exception; };
    class DecryptionError: public Exception { using Exception::Exception; };
    class InvalidKey     : public Exception { using Exception::Exception; };
} // namespace exc;

template <class... Args>
[[noreturn]] void FATAL(std::format_string<Args...> prompt, Args&&... args) 
{
    std::cerr << std::format(prompt, std::forward<Args>(args)...);
    exit(EXIT_FAILURE);
}

template <class... Args>
std::optional<std::string> input(std::format_string<Args...> prompt, Args&&... args)
{
    clearerr(stdin);
    std::cin.clear();

    std::string out;
    auto formatted = std::format(prompt, std::forward<Args>(args)...);

#ifdef __linux__
    char* line = readline(formatted.c_str()); 

    if (!line)
        return std::nullopt;

    add_history(line);
    out = std::string(line);
    free(line);
#else
    std::cout << formatted;

    if (!std::getline(std::cin, out)) {
        std::cout << "\n";
        return std::nullopt; 
    }

    std::cout << "\n";
#endif

    return out;
}

// Returns std::nullopt on EOF (ctrl + d)
template <class... Args>
std::optional<std::string> getpasswd(std::format_string<Args...> prompt, Args&&... args)
{
    clearerr(stdin);
    std::cin.clear();

    toggle_stdin_echo(false);
    std::cout << std::format(prompt, std::forward<Args>(args)...);

    std::string out;
    if (!std::getline(std::cin, out)) {
        std::cout << "\n";
        toggle_stdin_echo(true);
        return std::nullopt; 
    }

    std::cout << "\n";
    toggle_stdin_echo(true);
    return out;
}