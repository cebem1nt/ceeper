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

    int get_triplet(const std::string& tag, bool get_login = false, bool do_print = false);

    int remove_triplet(const std::string& tag, bool force = false);

    int edit_triplet(const std::string& tag);

    int find_triplet(const std::string& part,bool do_show = false);
    int list_triplets(bool ntriplets = false, bool do_show = false);

    int generate_password(uint length = 16, bool no_letters = false,
                          bool no_special_syms = false, bool do_print = false);

    int gen_and_add_triplet(const std::string& tag, uint length = 16, bool no_letters = false, 
                            bool no_special_syms = false, bool do_print = false);

    int change_locker(const std::string& dest, bool is_abs = false, bool do_create = false);

    int generate_token(bool force = false);
    int print_locker(bool is_abs = false);

    int encrypt_file(const std::string& file, std::optional<std::string> dest);
    int decrypt_file(const std::string& file, std::optional<std::string> dest);
        
    int handle_args(argparse::ArgumentParser& p);
    int interactive_cli(argparse::ArgumentParser& p);
    int main(int argc, char** argv);

private:
    Ceeper& ceeper_;
};