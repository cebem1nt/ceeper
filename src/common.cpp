#include "common.hpp"

#ifdef WIN32
#   include <windows.h>
#else
#   include <termios.h>
#   include <unistd.h>
#endif

#include <iostream>
#include <random>
#include <algorithm>

// https://sqlpey.com/c++/cpp-cross-platform-stdin-echo-control/
void toggle_stdin_echo(bool enable) 
{
#ifdef WIN32
    // Windows Implementation using GetStdHandle and SetConsoleMode
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD console_mode;
    GetConsoleMode(handle, &console_mode);

    if (!enable)
        console_mode &= ~ENABLE_ECHO_INPUT; // Disable echo
    else
        console_mode |= ENABLE_ECHO_INPUT;  // Enable echo

    SetConsoleMode(handle, console_mode);

#else
    // POSIX/Unix Implementation using termios
    struct termios ts;
    // Get current settings
    tcgetattr(STDIN_FILENO, &ts);
    
    if (!enable)
        ts.c_lflag &= ~ECHO; // Disable echo flag
    else
        ts.c_lflag |= ECHO;  // Enable echo flag

    // Apply new settings immediately
    (void) tcsetattr(STDIN_FILENO, TCSANOW, &ts);
#endif
}

std::string input()
{
    std::string out;
    std::getline(std::cin, out);
    std::cout << '\n';
    return out;
}

std::string to_lower(std::string in) 
{
    std::transform(in.begin(), in.end(), in.data(), ::tolower);
    return in;
}

std::string trim_whitespace(std::string in)
{
    auto not_space = [](unsigned char ch) { 
        return !std::isspace(ch); 
    };

    auto begin = std::find_if(in.begin(), in.end(), not_space);
    auto end   = std::find_if(in.rbegin(), in.rend(), not_space).base();

    if (begin >= end) 
        return {};
    
    return std::string(begin, end);
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
