#include "extensions.hpp"

#include <chrono>
#include <print>

namespace fs = std::filesystem;
using namespace std::chrono;

static std::string utc_now() 
{
    auto now = floor<seconds>(system_clock::now());
    return std::format("{:%F %T}Z", now);
}

GitManager::GitManager(Ceeper& ceeper):
    AExtension(ceeper) 
{
    storage_dir_ = ceeper_.storage_dir_;
    git_dir_ = storage_dir_ / ".git";
}

void GitManager::subscribe() 
{
#ifdef DEBUG
    std::println("subscribe");
#endif

    if (!fs::exists(git_dir_)) {
        auto create = input(
            "No repo in storage dir found, would you like to create one? [Y/n] ");

        if (to_lower(create)[0] == 'n')
            return;

        init_repo();
    }

    ceeper_.subscribe("init", [this] { pull(); }, true);
    ceeper_.subscribe("exit", [this] { push(); }, false);
}

std::string GitManager::git_run(const std::string& in) 
{
    std::string cmd = "git -C " + storage_dir_.string() + " " + in;
    std::array<char, 512> buf;
    std::string out;

#ifdef DEBUG
    std::println("git_run: {}", cmd);
#endif

    FILE* pipe = popen(cmd.c_str(), "r");

    while (fgets(buf.data(), buf.size(), pipe)) {
        out += buf.data();
    }

    pclose(pipe);
    return out;
}

void GitManager::push() 
{
    auto changes = git("status --porcelain");
    
    if (!changes.empty()) {
        std::println("Syncronizing...");
        git("add -A");
        git("commit -m 'sync {}'", utc_now());
        git("push --force");
    }
}

void GitManager::pull() 
{
    git("pull");
}

void GitManager::init_repo() 
{
    git("init");
    git("add -A");
    git("commit -m init");
    git("branch -M main");

    while (true) {
        auto remote = input("Enter remote origin: ");
        
        if (!remote.ends_with(".git")) {
            std::println("Looks incorrect, try again!");
            continue;
        }

        git("remote add origin {}", remote);
        git("push -u origin main");
        break;
    }
}