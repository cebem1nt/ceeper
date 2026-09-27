#include "../include/argparse.hpp"

#include "backend.hpp"

class CLI
{
public:
    CLI(Ceeper& instance) :
        ceeper_(instance) {};

    void welcome();
    int auth();
    int registrate();
    int login();

    int add_triplet(const std::string& tag, bool show_password = false,
                    std::optional<std::string> password = std::nullopt);

    int get_triplet(const std::string& tag, bool get_login, bool do_print = false);
    int remove_triplet(const std::string& tag, bool force = false);
    int edit_triplet(const std::string& tag);

    int find_triplet(const std::string& part,bool do_show = false);
    int list_triplets(bool ntriplets, bool do_show = false);

    int generate_password(uint length, bool no_letters, bool no_special_syms, 
                          bool do_print = false);

    int gen_and_add_triplet(const std::string& tag, uint length, bool no_letters, 
                            bool no_special_syms, bool do_print = false);

    int change_locker(const std::string& dest, bool is_abs, bool do_create);
    int generate_token(bool force);
    int print_locker(bool is_abs);

    int encrypt_file(const std::string& file, std::optional<std::string> dest, bool do_remove);
    int decrypt_file(const std::string& file, std::optional<std::string> dest, bool do_remove);
        
    int handle_args(argparse::ArgumentParser& p);
    int interactive_cli(argparse::ArgumentParser& p);
    int main(int argc, char** argv);

private:
    Ceeper& ceeper_;
};