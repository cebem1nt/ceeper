#pragma once

#include <string>
#include <filesystem>

#define LK_HEADER_SIZE 64 // SHA256 hash char length

# if defined (__ANDROID__)
#    define PLATFORM "Android"
# elif defined (_WIN32)
#    define PLATFORM "Windows"
# elif defined (__APPLE__) || defined (__MACH__)
#    define PLATFORM "Darwin"
# else
#    define PLATFORM "Posix"
# endif

namespace fs = std::filesystem;

class CrossPlatform {
public:
    fs::path user_dir_;
    fs::path data_dir_;
    fs::path storage_dir_;

    std::string platform_;

    CrossPlatform(fs::path current_dir, bool is_portable);
};

class FileSystem : public CrossPlatform {
public:
    FileSystem(
        int salt_size, 
        int token_size, 
        char* program_name,
        bool is_portable = false
    );

    void change_locker_dir(fs::path dir, bool same_ok = false, bool is_storage_relative = true);
    const fs::path get_locker_dir(bool is_full = true);

private:
    int salt_size_, token_size_;
    fs::path token_file_;
    fs::path locker_file_;
    fs::path default_locker_file_;
    fs::path current_locker_file_; // Stores a path to currently used locker

    void sync_locker();
    void append_line_to_locker(std::string header, std::string content);
    void remove_line_from_locker(std::string header);
    std::string get_line_from_locker(std::string header);
};