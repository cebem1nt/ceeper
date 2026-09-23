#include "params.hpp"

#include "frontend.hpp"
#include "extensions.hpp"

int main(int argc, char** argv)
{
    auto ceeper = Ceeper(
        params::TOKEN_SIZE, params::SALT_SIZE, params::ITERATIONS,
        params::BACKEND, argv[0], params::IS_PORTABLE_BUILD);

    std::vector<std::unique_ptr<AExtension>> extensions;

    // Maybe we'll add other extenions in future... who knows
    for (const auto& ext : params::EXTENSIONS) {
        if (std::string(ext) == "GitManager")
            extensions.push_back(std::make_unique<GitManager>(ceeper));
    }

    for (const auto& ins : extensions) {
        ins->subscribe();
    }

    auto frontend = CLI(ceeper);

    return frontend.main(argc, argv);
}