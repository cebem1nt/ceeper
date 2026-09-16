#include "argparse.hpp"
#include "params.hpp"

#include "frontend.hpp"

int main(int argc, char** argv)
{
    argparse::ArgumentParser p(argv[0]);
    auto& mode = p.add_mutually_exclusive_group();
    
    mode.add_argument("-A").metavar("TAG")
        .help("Add a new triplet with given TAG");

    mode.add_argument("-G").metavar("TAG")
        .help("get password by TAG");

    mode.add_argument("-R").metavar("TAG")
        .help("remove triplet / triplets by TAG[s]")
        .nargs(argparse::nargs_pattern::at_least_one);

    mode.add_argument("-E").metavar("TAG")
        .help("interactively edit triplet by TAG[s]")
        .nargs(argparse::nargs_pattern::at_least_one);

    mode.add_argument("-F").metavar("PART")
        .help("find triplets by given tag PART[s]")
        .nargs(argparse::nargs_pattern::at_least_one);

    mode.add_argument("-L").flag()
        .help("list triplets");

    // 

    p.add_argument("-p", "--print").flag()
        .help("print to stdout instead of copying");

    p.add_argument("-s", "--show").flag()
        .help("do not hide passwords");

    p.add_argument("-n", "--num").flag()
        .help("show number of stored passwords");

    p.add_argument("-f", "--force").flag()
        .help("force action, don't prompt for confirmation");

    p.add_argument("-l")
        .nargs(argparse::nargs_pattern::optional).scan<'i', int>().default_value(16)
        .help("when using -G: return login instead of password. "
              "when using --gen, treat as length for generated password");

    p.add_argument("-g", "--gen").flag()
        .help("Generate a password and print it to stdout, if used with -A, store newly generated password");

    p.add_argument("-nl", "--no-letters").flag()
        .help("generate a password without any letters.");

    p.add_argument("-ns", "--no-symbols").flag()
        .help("generate a password without any special symbols.");

    // TODO Generate parser and everything from keeper.py:69

    auto ceeper = Ceeper(
        params::TOKEN_SIZE, params::SALT_SIZE, params::ITERATIONS,
        params::BACKEND, argv[0], params::IS_PORTABLE_BUILD);

    auto frontend = CLI(ceeper);

    if (argc > 0) {
        try {
            p.parse_args(argc, argv);
        } catch (const std::exception& err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }

        return frontend.main(p);
    }

    return frontend.main(p, true); // interactive mode
}