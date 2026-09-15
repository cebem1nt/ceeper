#include "backend.hpp"
#include "argparse.hpp"

class CLI
{
public:
    CLI(Ceeper& instance) :
        ceeper_(instance) {};

    void welcome();
    void auth();
    void registrate();
    void login();

    int add_triplet(
        const std::string& tag,
        bool show_password = false,
        std::optional<std::string> password = std::nullopt 
    );

    int get_triplet(
        const std::string& tag,
        bool get_login = false,
        bool do_print = false
    );

    int remove_triplet(
        const std::string& tag,
        bool force = false
    );

    int edit_triplet(
        const std::string& tag
    );

    int find_triplet(
        const std::string& part,
        bool do_show = false
    );

    int list_triplets(
        bool ntriplets = false,
        bool do_show = false
    );

    int match_args(argparse::ArgumentParser& p);
    int interactive_cli(argparse::ArgumentParser& p);
    int main(argparse::ArgumentParser& p, bool is_interactive = false);

private:
    Ceeper& ceeper_;
};