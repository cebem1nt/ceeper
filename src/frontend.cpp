#include "frontend.hpp"

#ifdef WIN32
#   include <windows.h>
#else
#   include <termios.h>
#   include <unistd.h>
#endif

#include <iostream>
#include <print>

using std::println;
using std::print;

// https://sqlpey.com/c++/cpp-cross-platform-stdin-echo-control/
static void toggle_stdin_echo(bool enable = true) 
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

static std::string input(std::optional<std::string> prompt = std::nullopt) 
{
    if (prompt)
        std::cout << *prompt;

    std::string out;
    std::getline(std::cin, out);
    std::cout << '\n';

    return out;
}

template <class... Args>
static std::string getpasswd(std::format_string<Args...> prompt, Args&&... args)
{
    std::cout << std::format(prompt, std::forward<Args>(args)...);
    
    toggle_stdin_echo(false);
    auto out = input();
    toggle_stdin_echo(true);

    return out;
}

void CLI::welcome() 
{
    println("Yapi, hello world! welcome");
}

void CLI::auth() 
{
    if (ceeper_.get_locker_salt().empty())
        registrate();
    else
        login();
}

void CLI::registrate()  
{
    if (!ceeper_.token_exists())
        welcome();

    println("\nCurrent locker: {}", ceeper_.get_current_locker());

    while (true) {
        auto passwd = getpasswd("Create passphrase for the locker: ");

        if (passwd.length() <= 5) {
            println("Passphrase is too short");
            continue;
        }

        auto repeated = getpasswd("Repeat passphrase: ");
        
        if (passwd == repeated) {
            println("Passphrase created!"); 
            ceeper_.unlock(passwd);
            return;
        }

        println("Passphrases didn't match, try again");
    }
}

void CLI::login() 
{
    auto locker = ceeper_.get_current_locker(false);

    while (true) {
        auto passwd = getpasswd("Passphrase: [{}]", locker);

        try {
            if (ceeper_.unlock(passwd))
                return;
            else
                println("Incorrect passphrase, try again!");
        } catch (exc::NotInitialized& e) {
            println("You dont have a token yet. Generate it by 'ceeper generate-token' or set it manually to {}", 
                    ceeper_.token_file_.string());

            exit(1); // TODO see how handle exits correctly in interactive mode
        }
    }
}