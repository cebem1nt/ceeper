#include "filesystem.hpp"
#include "common.hpp"

#include <csignal>
#include <iostream>
#include <unordered_map>
#include <fstream>

namespace fs = std::filesystem;

static fs::path get_home() 
{
    std::string home;

#if defined (_WIN32)
    home = getenv("USERPROFILE"); 
#else
    home = getenv("HOME"); 
#endif
    return fs::path(home);
}

static fs::path get_pathenv(const char* name, fs::path fallback) 
{
    char* env = getenv(name);
    
    if (env == nullptr) 
        return fallback;
    
    fs::path env_path(env);
    
    if (!fs::exists(env_path))
        return fallback;

    return env_path;
}

static void create_file(fs::path& path) 
{
    std::ofstream(path).close();
}

static fs::path expand_user(fs::path in) 
{
    auto in_str = in.string();
    auto home = get_home();

    if (in_str == "~")
        return home;

    std::string prefix = "~"; 
    prefix += fs::path::preferred_separator;

    if (in_str.starts_with(prefix))
        return home / in_str.substr(prefix.size());

    return in;
}

namespace salt {
    bool exists(int salt_size, fs::path dir) 
    { 
        auto f = std::ifstream(dir, std::ios::binary);
        auto salt = std::string(salt_size, '\0');

        if (!f)
            return false;

        f.read(salt.data(), salt.size());
        
        if (f.gcount() != salt_size)
            return false;

        return true;
    }

    std::string generate(int salt_size, fs::path dir) 
    {
        auto salt = surandom(salt_size);
        auto f = std::ofstream(dir, std::ios::binary);

        f << salt;

        fs::permissions(dir, 
            fs::perms::owner_read | fs::perms::owner_write);

        return salt;
    }

    std::string get(int salt_size, fs::path dir, bool auto_gen = true) 
    {
        if (!salt::exists(salt_size, dir)) {
            if (auto_gen)
                return salt::generate(salt_size, dir);
            return "";
        }

        auto f = std::ifstream(dir, std::ios::binary);
        auto salt = std::string(salt_size, '\0');   
        
        f.read(salt.data(), salt.size());

        return salt;
    }
} // namespace salt

CrossPlatform::CrossPlatform(fs::path current_dir, bool is_portable) 
{
    fs::path default_stroage_dir; 
    fs::path prefered_storage_dir;

    platform_ = PLATFORM;
    user_dir_ = get_home();
    
    default_stroage_dir = user_dir_ / ".keeper_storage";
    prefered_storage_dir = get_pathenv("KEEPER_STORAGE_DIR", default_stroage_dir);

    if (is_portable) {
        platform_ = "Portable";
        storage_dir_ = current_dir / "storage";
    } else {
        storage_dir_ = prefered_storage_dir;
    }

    std::unordered_map<std::string, fs::path> data_dirs{
        { "Portable", current_dir / "data"                             },
        { "Posix",    user_dir_   / ".local" / "share" / "keeper"      },
        { "Android",  user_dir_   / ".local" / "share" / "keeper"      },
        { "Darwin",   user_dir_   / ".local" / "share" / "keeper"      },
        { "Windows",  get_pathenv("LOCALAPPDATA", user_dir_) / "keeper"},
    };

    auto it = data_dirs.find(platform_);

    if (it != data_dirs.end())
        data_dir_ = it->second;
    else 
        FATAL("Unsupported operating system \"%s\"!\n", platform_.c_str());

    fs::create_directories(data_dir_);
    fs::create_directories(storage_dir_);
}

        
FileSystem::FileSystem(int salt_size, int token_size, 
                       const char* program, bool is_portable)
    : CrossPlatform(fs::path(program).parent_path(), is_portable)
{
    salt_size_ = salt_size;
    token_size_ = token_size;

    token_file_ = data_dir_ / "token";
    current_locker_file_ = data_dir_ / "current_locker";
    default_locker_file_ = storage_dir_ / "default.lk";

    if (!fs::exists(current_locker_file_))
        create_file(current_locker_file_);

    sync_locker();
}

const fs::path FileSystem::get_locker_dir(bool is_full) 
{
    std::string content;

    auto f = std::ifstream(current_locker_file_);
    std::getline(f, content);

    auto cur_locker_dir = fs::path(content);

    if (!is_full && content.starts_with(storage_dir_.string()))
        return fs::relative(cur_locker_dir, storage_dir_);

    return cur_locker_dir;
}

void FileSystem::sync_locker() 
{
    locker_file_ = get_locker_dir();
   
    if (locker_file_.empty() || !fs::exists(locker_file_)) {
        locker_file_ = default_locker_file_;

        if (!fs::exists(locker_file_))
            create_file(locker_file_);

        change_locker_dir("default.lk");
    }
}

void FileSystem::change_locker_dir(fs::path dir, bool same_ok, 
                                   bool is_storage_relative) 
{
    fs::path dest;
    
    if (!is_storage_relative)
        dest = fs::absolute(expand_user(dir));
    else
        dest = storage_dir_ / dir;

    if (!fs::exists(dest)) {
        if (dest == default_locker_file_)
            create_file(default_locker_file_);
        else
            throw "Could not find locker file!";
    }

    if (fs::is_directory(dest))
        throw "Argument is a directory!";

    if (!same_ok && dest == get_locker_dir())
        throw "Same locker file!";

    auto f = std::ofstream(current_locker_file_);
    f << dest;

    sync_locker();
}

void FileSystem::append_line_to_locker(std::string header, std::string content) 
{
    auto f = std::ofstream(locker_file_, std::ios::binary | std::ios::app);
    f << header << content << '\n';
}

std::string FileSystem::get_line_from_locker(std::string header) 
{
    auto f = std::ifstream(locker_file_, std::ios::binary);
    f.seekg(salt_size_, std::ios::beg);
    
    std::string line;

    while (std::getline(f, line)) {
        if (line.compare(0, LK_HEADER_SIZE, header) == 0)
            return line; 
    }

    return "";
}

std::vector<std::string> FileSystem::get_all_lines_from_locker() 
{
    auto lines = std::vector<std::string>();
    auto f = std::ifstream(locker_file_, std::ios::binary);

    f.seekg(salt_size_, std::ios::beg);

    std::string line;
    while (std::getline(f, line)) {
        lines.push_back(line.substr(LK_HEADER_SIZE));
    }
    
    return lines;
}

void FileSystem::remove_line_from_locker(std::string header) 
{
    auto tmp_file = fs::path(locker_file_.string() + ".tmp");
    
    auto tmpf = std::ofstream(tmp_file, std::ios::binary);
    auto originalf = std::ifstream(locker_file_, std::ios::binary);

    auto salt = std::string(salt_size_, '\0');
    originalf.read(salt.data(), salt.size());

    std::string line;

    while (std::getline(originalf, line)) {
        if (line.compare(0, LK_HEADER_SIZE, header) != 0)
            tmpf << line << '\n';
    }

    originalf.close();

    auto previous = std::signal(SIGINT, SIG_IGN);
    fs::rename(tmp_file, locker_file_);
    std::signal(SIGINT, previous);
}

void FileSystem::copy_locker(fs::path dest) 
{
    fs::copy(locker_file_, fs::absolute(expand_user(dest)));
}

void FileSystem::copy_token(fs::path dest) 
{
    fs::copy(token_file_, fs::absolute(expand_user(dest)));
}

std::string FileSystem::get_locker_salt() 
{
    return salt::get(salt_size_, locker_file_);
}

std::string FileSystem::get_token() 
{
    return salt::get(token_size_, token_file_, false);
}

std::string FileSystem::generate_token()
{
    if (!fs::exists(token_file_) || fs::file_size(token_file_) == 0)
        return salt::generate(token_size_, token_file_);
    throw "Token allready exists!";
}

bool FileSystem::token_exists() 
{
    return salt::exists(token_size_, token_file_);
}

