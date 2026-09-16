#include "argparse.hpp"
#include "params.hpp"

#include "frontend.hpp"

int main(int argc, char** argv)
{
    argparse::ArgumentParser p(argv[0]);
    auto& mode = p.add_mutually_exclusive_group();
    
    // modes

    mode.add_argument("-A").help("Add a new triplet with given TAG")
        .metavar("TAG");

    mode.add_argument("-G").help("get password by TAG")
        .metavar("TAG");

    mode.add_argument("-R").help("remove triplet / triplets by TAG[s]")
        .nargs(argparse::nargs_pattern::at_least_one)
        .metavar("TAG");

    mode.add_argument("-E").help("interactively edit triplet by TAG[s]")
        .nargs(argparse::nargs_pattern::at_least_one)
        .metavar("TAG");

    mode.add_argument("-F").help("find triplets by given tag PART[s]")
        .nargs(argparse::nargs_pattern::at_least_one)
        .metavar("PART");

    mode.add_argument("-L").help("list triplets").flag();

    mode.add_argument("-C").help("changes current locker file to LOCKER (relative to storage dir by default)")
        .metavar("LOCKER");

    // Arguments / flags

    p.add_argument("-p", "--print").flag()
        .help("print to stdout instead of copying");

    p.add_argument("-l")
        .nargs(argparse::nargs_pattern::optional).scan<'i', int>().default_value(16)
        .help("when using -G: return login instead of password. "
              "when using --gen, treat as length for generated password");

    p.add_argument("-s", "--show")       .help("do not hide passwords").flag();
    p.add_argument("-n", "--num")        .help("show number of stored passwords").flag();
    p.add_argument("-c", "--current")    .help("print current locker path").flag();
    p.add_argument("-f", "--force")      .help("force action, don't prompt for confirmation").flag();
    p.add_argument("-a", "--absolute")   .help("treat given locker paths as non relative to storage dir").flag();

    p.add_argument("-g",  "--gen")       .help("generate a password and copy it, if used with -A, store newly generated password").flag();
    p.add_argument("-nl", "--no-letters").help("generate a password without any letters.").flag();
    p.add_argument("-ns", "--no-symbols").help("generate a password without any special symbols.").flag();

    p.add_argument("--generate-token")   .help("generate a new token").flag();

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