#include "frontend.hpp"

#include "clipcpy.hpp"
#include "common.hpp"

#include <iostream>
#include <print>

#define ARGS_GET_STRVEC(p, flag) p.get<std::vector<std::string>>(flag)
#define ARGS_GET_STR(p, flag) p.get<std::string>(flag)

using std::println;

void print_triplet(const ceeper::Triplet& t, bool show_password = false) 
{
    println("\nTag: {}", t[0]);
    println("Login: {}", t[1]);

    if (show_password)
        println("Password: {}", t[2]);
}

void CLI::welcome() 
{
    println("Yapi, hello world! welcome");
}

void CLI::auth() 
{
    if (ceeper_.is_new_locker())
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
        auto passwd = getpasswd("Passphrase: [{}] ", locker);

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

int CLI::get_triplet(const std::string& tag,
                     bool get_login, bool do_print) 
{
    auto triplet = ceeper_.get_triplet(tag);
    
    if (!triplet) {
        println("Could not find triplet with tag: {}", tag);
        return 1;
    }

    auto out = get_login ? (*triplet)[1] 
                         : (*triplet)[2];

    if (do_print) {
        std::cout << out << '\n';
        return 0;
    }
        
    bool success = clipcpy(out);

    if (success)
        println("Added to clipboard!");
    else
        println("Could not copy, unsupported os for now :(");

    return 0;
}

int CLI::remove_triplet(const std::string& tag, bool force) 
{
    auto triplet = ceeper_.get_triplet(tag);

    if (!triplet) {
        println("Could not find triplet with tag: {}", tag);
        return 1;
    }

    print_triplet(*triplet);
    putchar('\n');

    if (!force) {
        auto choice = input("Remove this triplet? [y/N] ");
        choice = to_lower(trim_whitespace(choice));
        
        if (choice != "y") {
            println("Aboarting..");
            return 1;
        }
    }

    ceeper_.remove_triplet(tag);
    println("Triplet removed");

    return 0;
}

int CLI::list_triplets(bool ntriplets, bool do_show)
{
    auto triplets = ceeper_.list_triplets();
    
    if (ntriplets)
        println("{}", triplets.size()); 
    else {
        for (auto& t : triplets) {
            print_triplet(t, do_show);
        }
    }

    return 0;
}

int CLI::match_args(argparse::ArgumentParser& p)
{
    if (!ceeper_.is_unlocked())
        auth();

    int rcs = 0;

    if (p.is_used("-A")) {
        for (auto& tag : ARGS_GET_STRVEC(p, "-A")) 
            rcs += add_triplet(tag, p.get<bool>("-s"));
        
        return rcs;
    } else if (p.is_used("-G")) {
        return get_triplet(
            ARGS_GET_STR(p, "-G"), 
            p.get<bool>("-l"), p.get<bool>("-p"));

    } else if (p.is_used("-R")) {
        for (auto& tag : ARGS_GET_STRVEC(p, "-R")) 
            rcs += remove_triplet(tag, p.get<bool>("-f"));
        
        return rcs;
    } else if (p.is_used("-L")) {
        return list_triplets(p.get<bool>("-n"), 
                             p.get<bool>("-s"));
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
}