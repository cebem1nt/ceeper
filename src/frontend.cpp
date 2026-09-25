#include "../include/clipcpy.hpp"

#include "frontend.hpp"
#include "common.hpp"

#include <csignal>
#include <iostream>
#include <print>
#include <unistd.h>

using std::println;
using nargs = argparse::nargs_pattern;

static void on_sigint(int)
{
    // It is a big pain in the ass to correctly handle SIGINT in interactive prompt
    // AND triger "exit" event, so we'll just kindly ask to type quit
    if (rl_readline_state & RL_STATE_READCMD) {
        println("CTRL+D or 'exit' to exit");
        rl_on_new_line(); // Regenerate the prompt on a newline
        rl_replace_line("", 0); // Clear the previous text
        rl_redisplay();
    }
}

static void setup_signals() 
{
// TODO is windows alternative needed?
#ifndef _WIN32
    std::signal(SIGINT, on_sigint);
#endif
}

void print_triplet(const ceeper::Triplet& t, bool show_password = false) 
{
    println("\nTag: {}", t[0]);
    println("Login: {}", t[1]);

    if (show_password)
        println("Password: {}", t[2]);
}

void CLI::welcome() 
{
    clear_screen();
    println(
        "          \033[36m/\\ /\\___  \033[35m___ _ __   ___ _ __    \n"
        "         \033[36m/ //_/ _ \\\033[35m/ _ \\ '_ \\ / _ \\ '__| \n"
        "        \033[36m/ __ \\  __/\033[35m  __/ |_) |  __/ |       \n"
        "        \033[36m\\/  \\/\\___|\033[35m\\___| .__/ \\___|_|   \n"
        "                    \033[35m |_|                             \n"
        "            \033[0m"
    );

    println("Keeper is a password manager designed to securely store your passwords locally.");
    println("Each password file is referred to as a \033[33m\"locker\"\033[0m and has a .lk extension.");
    println("You can manage multiple lockers, each containing different sets of passwords.");
    println("Passwords are stored in a triplet format: \033[33mtag/login/password\033[0m.");
    println("Use the tag to retrieve detailed information about each triplet.");
    println("Lockers are encrypted with a passphrase and your unique token.");
    println("If you want to use your lockers on multiple devices, you need the same token.\n");

    println("\033[33m[WARNING!]\033[0m Make sure to remember this passphrase! as losing it");
    println("means you will not be able to recover your encrypted passwords.\n");

    println("\033[32m[INFO]\033[0m You don't have a token yet. Generate it by \033[4m\"cee --generate-token\"\033[0m");
    println("or insert your existing token to {}\n", ceeper_.token_file_.string());

    exit(0);
}

int CLI::auth() 
{
    if (ceeper_.is_new_locker())
        return registrate();
    else
        return login();
}

int CLI::registrate()  
{
    if (!ceeper_.token_exists())
        welcome();

    println("Current locker: {}", ceeper_.get_current_locker());

    while (true) {
        auto passwd = getpasswd("Create passphrase for the locker: ");
        if (!passwd)
            return 1;

        if (passwd->length() <= 5) {
            println("Passphrase is too short");
            continue;
        }

        auto repeated = getpasswd("Repeat passphrase: ");
        
        if (passwd == repeated) {
            println("Passphrase created!"); 
            ceeper_.unlock(*passwd);
            return 0;
        }

        println("Passphrases didn't match, try again");
    }

    return 1;
}

int CLI::login() 
{
    auto locker = ceeper_.get_current_locker(false);

    while (true) {
        auto passwd = getpasswd("Passphrase: [{}] ", locker);
        if (!passwd)
            return 1;

        try {
            if (ceeper_.unlock(*passwd))
                return 0;
            else
                println("Incorrect passphrase, try again!");
        } catch (exc::NotInitialized& e) {
            println("You dont have a token yet. Generate it with --generate-token or set it manually to {}", 
                    ceeper_.token_file_.string());
            exit(1);
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
    
    std::optional<std::string > login;

    while (true) {
        login = input("Enter login: ");
        if (!login)
            return 1;

        if (!login->empty())
            break;

        println("Login can not be empty!");
    }

    if (!password) {
        while (true) {
            if (show_password)
                password = input("Enter the password [*] : ");
            else
                password = getpasswd("Enter the password: ");

            if (password && !password->empty())
                break;

            println("Password can not be empty!");
        }
    }

    ceeper_.store_triplet({ tag, *login, *password });
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
        if (!choice)
            return 1;

        choice = to_lower(trim_whitespace(*choice));
        
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
        if (!in)
            return 1;

        try {
            prop = std::stoi(*in);
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
        if (!value)
            return 1;

        try {
            ceeper_.edit_triplet(tag, prop, *value);
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

int CLI::change_locker(const std::string& dest, bool is_abs, bool do_create)
{
    try {
        ceeper_.change_locker_dir(std::filesystem::path(dest), false, !is_abs, do_create);
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

int CLI::encrypt_file(const std::string& file, std::optional<std::string> dest)
{
    while (true) {
        auto passwd = getpasswd("Create encryption passphrase: ");

        if (!passwd)
            return 1; 

        if (passwd->empty()) {
            println("Passphrase can not be empty!");
            continue;
        }

        try {
            ceeper_.file_encrypt(*passwd, file, dest);
        } catch (exc::AlreadyExists& e) {
            println("{}", e.what());
            println("Hint: use -o to provide output file");
            return 1;
        }
    }

    return 0;
}

int CLI::decrypt_file(const std::string& file, std::optional<std::string> dest)
{
    while (true) {
        auto passwd = getpasswd("Enter decryption passphrase: ");
    
        if (!passwd)
            return 1; 

        if (passwd->empty()) {
            println("Passphrase can not be empty!");
            continue;
        }        

        try {
            ceeper_.file_decrypt(*passwd, file, dest);
        } catch (exc::AlreadyExists& e) {
            println("{}", e.what());
            println("Hint: use -o to provide output file");
            return 1;
        } catch (exc::DecryptionError) {
            println("Incorrect passphrase!");
            return 1;
        }
    }

    return 0;
}

int CLI::interactive_cli(argparse::ArgumentParser& p) 
{
    setup_signals();
    auto currnet_locker = ceeper_.get_current_locker(); // We need this to know when to re-auth

    while (true) {
        if (currnet_locker != ceeper_.get_current_locker() || ceeper_.is_new_locker()) {
            currnet_locker = ceeper_.get_current_locker();
            auth();
        }

        auto in = input(">> ");
        if (!in) // EOF
            return 0;

        const auto cmd = trim_whitespace(*in);

        if (cmd.empty())
            continue;

        if (cmd == "quit" || cmd == "exit") {
            println("Exiting...");
            break;
        } 
        
        if (cmd == "clear") {
            clear_screen();
            continue;
        }

        auto args = splitstr(cmd);
        args.insert(args.begin(), "program");

        try {
            p.wipe();
            p.parse_args(args);
        } catch (std::exception& e) {
            println("{}", e.what());
            continue;
        }

        handle_args(p);
    }

    return 0;
}

int CLI::handle_args(argparse::ArgumentParser& p)
{
    // These can be executed without auth
    if (p.is_used("--generate-token"))
        return generate_token(p.getb("-f")); 
    
    if (p.is_used("-c"))
        return print_locker(p.getb("-a"));

    if (auto* s = p.get_subparser({"change", "ch", "c"}))
        return change_locker(s->get("locker"), s->getb("-a"), s->getb("-m"));
    
    if (p.is_used("-g"))
        return generate_password(p.geti("-l"), p.getb("-nl"), p.getb("-ns"), p.getb("-p"));

    if (p.is_used("-e"))
        return encrypt_file(p.get("-e"), p.present("-o"));

    if (p.is_used("-d"))
        return decrypt_file(p.get("-d"), p.present("-o"));

    int rc = 0;

    if (!ceeper_.is_unlocked())
        if (auth() != 0)
            return 1;
    
    if (auto* s = p.get_subparser({"add", "a"})) {
        auto tag = s->get("tag");
        return s->is_used("-g")
                 ? gen_and_add_triplet(tag, s->geti("-l"), s->getb("-nl"), s->getb("-ns"), s->getb("-p"))
                 : add_triplet(tag, s->getb("-s"));
    }

    if (auto* s = p.get_subparser({"get", "g"})) {
        return get_triplet(s->get("tag"), s->getb("-l"), s->getb("-p"));
    }

    if (auto* s = p.get_subparser({"remove", "rm", "r"})) {
        for (auto& tag : s->getstrv("tag"))
            rc += remove_triplet(tag, s->getb("-f"));
        return rc;
    }

    if (auto* s = p.get_subparser({"edit", "e"})) {
        for (auto& tag : s->getstrv("tag"))
            rc += edit_triplet(tag);
        return rc;
    }

    if (auto* s = p.get_subparser({"find", "f"})) {
        for (auto& part : s->getstrv("part"))
            rc += find_triplet(part);
        return rc;
    }

    if (auto* s = p.get_subparser({"list", "ls", "l"}))
        return list_triplets(s->getb("-n"), s->getb("-s"));

    return 0;
}

int CLI::main(int argc, char** argv)
{
    auto p = argparse::ArgumentParser(argv[0]);

    auto add_p = argparse::ArgumentParser("add");
    add_p.add_description("add a new triplet with given TAG");
    add_p.add_argument("tag").metavar("TAG") .help("tag for new triplet");
    add_p.add_argument("-s", "--show")       .help("do not hide passwords").flag();
    add_p.add_argument("-g",  "--gen")       .help("generate a password, copy, and store it with tag").flag();
    add_p.add_argument("-nl", "--no-letters").help("generate a password without any letters.").flag();
    add_p.add_argument("-ns", "--no-symbols").help("generate a password without any special symbols.").flag();
    add_p.add_argument("-p", "--print")      .help("print to stdout instead of copying").flag();
    add_p.add_argument("-l",  "--length")    .help("length for newly generated password").nargs(nargs::optional).scan<'i', int>().default_value(16);
    p.add_subparser({"a"}, add_p);

    auto get_p = argparse::ArgumentParser("get");
    get_p.add_description("get password by TAG");
    get_p.add_argument("tag").metavar("TAG").help("tag of the triplet");
    get_p.add_argument("-l", "--login") .help("return login instead of password.").flag();
    get_p.add_argument("-p", "--print") .help("print to stdout instead of copying").flag();
    p.add_subparser({"g"}, get_p);

    auto remove_p = argparse::ArgumentParser("remove");
    remove_p.add_description("remove triplet[s] by TAG[s]");
    remove_p.add_argument("tag").metavar("TAG ...").help("tag[s] of triplet[s] to remove").nargs(nargs::at_least_one);
    remove_p.add_argument("-f", "--force") .help("force action, don't prompt for confirmation").flag();
    p.add_subparser({"r", "rm"}, remove_p);

    auto edit_p = argparse::ArgumentParser("edit");
    edit_p.add_description("interactively edit triplet[s] by TAG[s]");
    edit_p.add_argument("tag").metavar("TAG ...").help("tag[s] of triplet[s] to edit").nargs(nargs::at_least_one);
    p.add_subparser({"e"}, edit_p);

    auto find_p = argparse::ArgumentParser("find");
    find_p.add_description("look for triplets by given tag PART[s]");
    find_p.add_argument("part").metavar("PART ...").help("part[s] of triplet tag to look for").nargs(nargs::at_least_one);
    p.add_subparser({"f"}, find_p);

    auto list_p = argparse::ArgumentParser("list");
    list_p.add_description("list triplets");
    list_p.add_argument("-n", "--num")  .help("show number of stored passwords").flag();
    list_p.add_argument("-s", "--show") .help("do not hide passwords").flag();
    p.add_subparser({"l", "ls"}, list_p);

    auto change_p = argparse::ArgumentParser("change");
    change_p.add_description("changes current locker file to LOCKER");
    change_p.add_argument("locker").metavar("LOCKER").help("new locker file path");
    change_p.add_argument("-m", "--make")     .help("create locker file if does not exist").flag();
    change_p.add_argument("-a", "--absolute") .help("treat locker paths as non relative to storage dir").flag();
    p.add_subparser({"c", "ch"}, change_p);

    p.add_argument("-c", "--current")     .help("print current locker path").flag();
    p.add_argument("-f", "--force")       .help("force action, don't prompt for confirmation").flag();
    p.add_argument("-a", "--absolute")    .help("treat locker paths as non relative to storage dir").flag();
    p.add_argument("-p", "--print")       .help("print to stdout instead of copying").flag();

    p.add_argument("-g",  "--gen")        .help("generate a password, copy, and store it with tag").flag();
    p.add_argument("-nl", "--no-letters") .help("generate a password without any letters.").flag();
    p.add_argument("-ns", "--no-symbols") .help("generate a password without any special symbols.").flag();
    p.add_argument("-l",  "--length")     .help("length for newly generated password").nargs(nargs::optional).scan<'i', int>().default_value(16);
    p.add_argument("--generate-token")    .help("generate a new token").flag();

    p.add_argument("-e", "--encrypt").metavar("FILE") .help("prompts for password, encrypts given file");
    p.add_argument("-d", "--decrypt").metavar("FILE") .help("prompts for password, decrypts given file");
    p.add_argument("-o", "--out").metavar("OUT")      .help("when using -e/-d use this as dest file");

    p.add_epilog(
        "Each password file is referred to as a \033[33m\"locker\"\033[0m and has a .lk extension.\n"
        "You can manage multiple lockers, each containing different sets of passwords.\n"
        "Passwords are stored in a triplet format: \033[33mtag/login/password\033[0m.\n"
        "Use the tag to retrieve detailed information about each triplet.\n"
        "Lockers are encrypted with a passphrase and your unique token.\n"
        "If you want to use your lockers on multiple devices, you need the same token."
    );

    int rc = 0;

    if (argc == 1)
        rc = interactive_cli(p); // interactive mode
    else {
        try {
            p.parse_args(argc, argv);
        } catch (argparse::help_exception& e) {
            println("{}", e.what());
            return 0;
        } catch (const std::exception& err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }

        rc = handle_args(p);
    }

    ceeper_.trigger_event("exit");
    return rc;
}