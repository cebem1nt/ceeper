#include "../include/clipcpy.hpp"

#include "frontend.hpp"
#include "common.hpp"

#include <csignal>
#include <iostream>
#include <print>
#include <unistd.h>

static std::vector<std::unique_ptr<argparse::ArgumentParser>> ALIAS_STORAGE;

#define ARGPARSE_ADD_ALIAS(subparser, parser, ...)                                      \
    for (const auto& alias : __VA_ARGS__) {                                             \
        ALIAS_STORAGE.emplace_back(std::make_unique<argparse::ArgumentParser>(alias));  \
        auto& ap = *ALIAS_STORAGE.back();                                               \
        ap.add_parents(subparser);                                                      \
        ap.set_suppress(true);                                                          \
        parser.add_subparser(ap);                                                       \
    }

static argparse::ArgumentParser* get_subparser(argparse::ArgumentParser& parser,
                                               std::vector<std::string> names) 
{
    for (auto& name : names) {
        if (parser.is_subcommand_used(name))
            return &parser.at<argparse::ArgumentParser>(name);
    }

    return nullptr;
}

using std::println;

static void clear_screen() 
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void on_sigint(int)
{
    // It is a big pain in the ass to correctly handle SIGINT in interactive prompt
    // AND triger "exit" event, so we'll just kindly ask to type quit
    println("Type quit or exit");
    rl_on_new_line(); // Regenerate the prompt on a newline
    rl_replace_line("", 0); // Clear the previous text
    rl_redisplay();
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

    println("Current locker: {}", ceeper_.get_current_locker());

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

int CLI::handle_args(argparse::ArgumentParser& p)
{
    // These can be executed without auth
    if (p.is_used("--generate-token"))
        return generate_token(p.getb("-f")); 
    
    if (p.is_used("-c"))
        return print_locker(p.getb("-a"));

    if (auto* subp = get_subparser(p, {"change", "ch", "c"}))
        return change_locker(subp->get("locker"), p.getb("-a"));
    
    if (p.is_used("-g") && !get_subparser(p, {"add", "a"}))
        return generate_password(p.geti("-l"), p.getb("-nl"), p.getb("-ns"), p.getb("-p"));

    if (!ceeper_.is_unlocked())
        auth();

    int rc = 0;

    if (auto* subp = get_subparser(p, {"add", "a"})) {
        auto tag = subp->get("tag");
        return p.is_used("-g")
                 ? gen_and_add_triplet(tag, p.geti("-l"), p.getb("-nl"), p.getb("-ns"), p.getb("-p"))
                 : add_triplet(tag, p.getb("-s"));
    }

    if (auto* subp = get_subparser(p, {"get", "g"})) {
        return get_triplet(subp->get("tag"), subp->getb("-l"), p.getb("-p"));
    }

    if (auto* subp = get_subparser(p, {"remove", "rm", "r"})) {
        for (auto& tag : subp->getstrv("tag"))
            rc += remove_triplet(tag, p.getb("-f"));
        return rc;
    }

    if (auto* subp = get_subparser(p, {"edit", "e"})) {
        for (auto& tag : subp->getstrv("tag"))
            rc += edit_triplet(tag);
        return rc;
    }

    if (auto* subp = get_subparser(p, {"find", "f"})) {
        for (auto& part : subp->getstrv("part"))
            rc += find_triplet(part);
        return rc;
    }

    if (auto* subp = get_subparser(p, {"list", "ls", "l"}))
        return list_triplets(subp->getb("-n"), p.getb("-s"));

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

        const auto cmd = trim_whitespace(input(">> "));

        if (cmd.empty())
            continue;

        if (cmd == "quit" || cmd == "exit") {
            println("Exiting...");
            break;
        } 
        
        if (cmd == "clear")
            clear_screen();

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

int CLI::main(int argc, char** argv) 
{
    auto p = argparse::ArgumentParser(argv[0]);
    
    auto add_p = argparse::ArgumentParser("add");
    add_p.add_description("add a new triplet with given TAG");
    add_p.add_argument("tag").help("tag for new triplet");
    p.add_subparser(add_p);
    ARGPARSE_ADD_ALIAS(add_p, p, {"a"});

    auto get_p = argparse::ArgumentParser("get");
    get_p.add_description("get password by TAG");
    get_p.add_argument("tag").help("tag of the triplet");
    get_p.add_argument("-l", "--login").help("return login instead of password.").flag();
    p.add_subparser(get_p);
    ARGPARSE_ADD_ALIAS(get_p, p, {"g"});

    auto remove_p = argparse::ArgumentParser("remove");
    remove_p.add_description("remove triplet[s] by TAG[s]");
    remove_p.add_argument("tag").help("tag[s] of triplet[s] to remove")
        .nargs(argparse::nargs_pattern::at_least_one);
    p.add_subparser(remove_p);
    ARGPARSE_ADD_ALIAS(remove_p, p, {"r", "rm"});

    auto edit_p = argparse::ArgumentParser("edit");
    edit_p.add_description("interactively edit triplet[s] by TAG[s]");
    edit_p.add_argument("tag").help("tag[s] of triplet[s] to edit")
        .nargs(argparse::nargs_pattern::at_least_one);
    p.add_subparser(edit_p);
    ARGPARSE_ADD_ALIAS(edit_p, p, {"e"});

    auto find_p = argparse::ArgumentParser("find");
    find_p.add_description("look for triplets by given tag PART[s]");
    find_p.add_argument("part").help("part[s] of triplet tag to look for")
        .nargs(argparse::nargs_pattern::at_least_one);
    p.add_subparser(find_p);
    ARGPARSE_ADD_ALIAS(find_p, p, {"f"});

    auto list_p = argparse::ArgumentParser("list");
    list_p.add_description("list triplets");
    list_p.add_argument("-n", "--num").help("show number of stored passwords").flag();
    p.add_subparser(list_p);
    ARGPARSE_ADD_ALIAS(list_p, p, {"l", "ls"});

    auto change_p = argparse::ArgumentParser("change");
    change_p.add_description("changes current locker file to LOCKER (relative to storage dir by default)");
    change_p.add_argument("locker").help("new locker file path");
    p.add_subparser(change_p);
    ARGPARSE_ADD_ALIAS(change_p, p, {"c", "ch"});

    // Arguments / flags

    p.add_argument("-p", "--print")      .help("print to stdout instead of copying").flag();
    p.add_argument("-s", "--show")       .help("do not hide passwords").flag();
    p.add_argument("-c", "--current")    .help("print current locker path").flag();
    p.add_argument("-f", "--force")      .help("force action, don't prompt for confirmation").flag();
    p.add_argument("-a", "--absolute")   .help("treat locker paths as non relative to storage dir").flag();

    p.add_argument("-g",  "--gen")       .help("generate a password and copy it, if used with -A, store newly generated password").flag();
    p.add_argument("-nl", "--no-letters").help("generate a password without any letters.").flag();
    p.add_argument("-ns", "--no-symbols").help("generate a password without any special symbols.").flag();
    p.add_argument("-l",  "--length")    .help("length for newly generated password")
        .nargs(argparse::nargs_pattern::optional).scan<'i', int>().default_value(16);

    p.add_argument("--generate-token")   .help("generate a new token").flag();

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