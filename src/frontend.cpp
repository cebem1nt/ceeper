#include "frontend.hpp"

#ifdef WIN32
#   include <windows.h>
#else
#   include <termios.h>
#   include <unistd.h>
#endif

#include <iostream>
#include <print>

#define ARGS_GET_STRVEC(p, flag) p.get<std::vector<std::string>>(flag)
#define ARGS_GET_STR(p, flag) p.get<std::string>(flag)

using std::println;

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
    if (!ceeper_.is_locker_salted())
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

int CLI::add_triplet(const std::string& tag, 
                     bool show_password,
                     std::optional<std::string> password)
{
    if (ceeper_.get_triplet(tag)) {
        println("Triplet with tag {} already exists.", tag);
        return 1;
    }

    println("\nCreating new triplet with tag \"{}\"\n", tag);
    
    std::string login;

    while (true) {
        login = input("Enter login: ");

        if (!login.empty())
            break;

        println("Login can not be empty!");
    }

    if (!password) {
        while (true) {
            if (show_password)
                password = input("Enter the password [*] : ");
            else
                password = getpasswd("Enter the password: ");

            if (!password->empty())
                break;

            println("Password can not be empty!");
        }
    }

    ceeper_.store_triplet({ tag, login, *password });
    println("Triplet successfully stored with tag: {}", tag);
    return 0;
}

int CLI::match_args(argparse::ArgumentParser& p)
{
    if (!ceeper_.is_unlocked())
        auth();

    if (p.is_used("-A")) {
        for (auto& tag : ARGS_GET_STRVEC(p, "-A")) {
            return add_triplet(tag, p.get<bool>("-s"));
        }
    }

    std::unreachable();
}

int CLI::interactive_cli(argparse::ArgumentParser& p) 
{
    std::unreachable();
}

int CLI::main(argparse::ArgumentParser& p, bool is_interactive) 
{
    // TODO events

    if (is_interactive)
        return interactive_cli(p);

    return match_args(p);

    return 0;
}