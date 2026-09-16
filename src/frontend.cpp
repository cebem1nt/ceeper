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

int CLI::add_triplet(const std::string& tag, bool show_password,
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

int CLI::edit_triplet(const std::string& tag)
{
    const char* params[] = {"Tag", "Login", "Password"};

    auto triplet = ceeper_.get_triplet(tag);
    int prop = 0;

    if (!triplet) {
        println("Could not find triplet with tag: {}", tag);
        return 1;
    }

    println("\nEditing triplet with tag: {}", tag);
    println("Parameters that can be edited: \n"
            "\t0 - tag \n"
            "\t1 - login \n"
            "\t2 - password\n");

    while (true) {
        auto in = input("Enter the parameter to edit (0-2): ");

        try {
            prop = std::stoi(in);
        } catch (...) {
            prop = -1;
        }

        if (prop < 0 || prop > 2) {
            println("Incorrect parameter, try again");
            continue;
        }

        break;
    }

    while(true) {
        auto value = input("Enter new value for \"{}\": ", params[prop]);

        try {
            ceeper_.edit_triplet(tag, prop, value);
            break;
        } catch (exc::AlreadyExists& e) {
            println("{}", e.what());
        }
    }

    println("Succesfully edited triplet with tag: \"{}\"", tag);
    return 0;
}


int CLI::find_triplet(const std::string& part, bool do_show) 
{
    auto found = ceeper_.search_for_triplets(part);
 
    for (auto& t : found) {
        print_triplet(t, do_show);
    }

    return 0;
}

int CLI::generate_password(uint length, bool no_letters, 
                           bool no_special_syms, bool do_print) 
{
    std::string generated = {};

    try {
        generated = ceeper_.generate_password(length, no_letters, no_special_syms);
    } catch (exc::ValueMismatch& e) {
        println("{}", e.what());
        return 1;
    }

    if (do_print)
        println("{}", generated);
    else {
        clipcpy(generated);
        println("Added to clipboard!");
    }

    return 0;
}

int CLI::gen_and_add_triplet(const std::string& tag, uint length, bool no_letters,
                             bool no_special_syms, bool do_print) 
{
    std::string generated = {};

    try {
        generated = ceeper_.generate_password(length, no_letters, no_special_syms);
    } catch (exc::ValueMismatch& e) {
        println("{}", e.what());
        return 1;
    }

    int rc = add_triplet(tag, false, generated);
    
    if (rc != 0)
        return rc;

    if (do_print) 
        println("{}", generated);
    else {
        clipcpy(generated);
        println("Added to clipboard!");
    }

    return 0;
}

int CLI::change_locker(const std::string& dest, bool is_abs)
{
    try {
        ceeper_.change_locker_dir(std::filesystem::path(dest), false, !is_abs);
        println("\nSuccesfuly changed current locker to: {}\n", dest);
    } catch (exc::Exception& e) {
        println("{}", e.what());
        return 1;
    }

    return 0;
}


int CLI::generate_token(bool force) 
{
    println("Generating token...");
    
    try {
        ceeper_.generate_token(force);
        println("Token was generated");
    } catch (exc::Exception& e) {
        println("{}", e.what());
        return 1;
    }

    return 0;
}


int CLI::print_locker(bool is_abs) 
{
    println("{}", ceeper_.get_current_locker(is_abs));
    return 0;
}

int CLI::match_args(argparse::ArgumentParser& p)
{
    if (!ceeper_.is_unlocked())
        auth();

    int rc = 0;

    if (auto tag = p.present("-A")) {
        return p.is_used("-g")
            ? gen_and_add_triplet(*tag, p.geti("-l"), p.getb("-nl"), p.getb("-ns"), p.getb("-p"))
            : add_triplet(*tag, p.getb("-s"));
    }

    if (auto tag = p.present("-G")) {
        return get_triplet(*tag, p.is_used("-l"), p.getb("-p"));
    }

    if (p.is_used("-R")) {
        for (auto& tag : p.getstrv("-R"))
            rc += remove_triplet(tag, p.getb("-f"));
        return rc;
    }

    if (p.is_used("-E")) {
        for (auto& tag : p.getstrv("-E"))
            rc += edit_triplet(tag);
        return rc;
    }

    if (p.is_used("-F")) {
        for (auto& part : p.getstrv("-F"))
            rc += find_triplet(part);
        return rc;
    }

    if (p.is_used("-L"))
        return list_triplets(p.getb("-n"), p.getb("-s"));

    if (auto dest = p.present("-C"))
        return change_locker(*dest, p.getb("-a"));

    if (p.is_used("-g"))
        return generate_password(p.geti("-l"), p.getb("-nl"), p.getb("-ns"), p.getb("-p"));

    if (p.is_used("--generate-token"))
        return generate_token(p.getb("-f"));

    if (p.is_used("-c"))
        return print_locker(p.getb("-a"));

    return 0;
}

int CLI::interactive_cli(argparse::ArgumentParser& p) 
{
    std::unreachable();
}

int CLI::main(argparse::ArgumentParser& p, bool is_interactive) 
{    
    if (is_interactive)
        return interactive_cli(p);

    // These can be executed without auth
    if (p.is_used("--generate-token"))
        return generate_token(p.getb("-f")); 
    
    if (auto dest = p.present("-C"))
        return change_locker(*dest, p.getb("-a"));
    
    if (p.is_used("-c"))
        return print_locker(p.getb("-a"));
    
    if (p.is_used("-g") && !p.is_used("-A"))
        return generate_password(p.geti("-l"), p.getb("-nl"), p.getb("-ns"), p.getb("-p"));

    return match_args(p);
}