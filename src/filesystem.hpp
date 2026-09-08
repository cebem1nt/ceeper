#pragma once

#include <string>
#include <filesystem>
#include <vector>

#define LK_HEADER_SIZE 64 // SHA256 hash char length

#if defined (__ANDROID__)
#    define PLATFORM "Android"
#elif defined (_WIN32)
#    define PLATFORM "Windows"
#elif defined (__APPLE__) || defined (__MACH__)
#    define PLATFORM "Darwin"
#elif defined(__linux__)
#    define PLATFORM "Posix"
#else
#    error Unsupported platform
#endif

class CrossPlatform 
{
public:
    std::filesystem::path user_dir_;
    std::filesystem::path data_dir_;
    std::filesystem::path storage_dir_;

    std::string platform_;

    CrossPlatform(std::filesystem::path current_dir, bool is_portable);
};

class FileSystem 
    : public CrossPlatform 
{
public:
    FileSystem(
        int salt_size, 
        int token_size, 
        const char* program_name,
        bool is_portable = false
    );

    int salt_size_, token_size_;
    std::filesystem::path token_file_;
    std::filesystem::path locker_file_;
    std::filesystem::path default_locker_file_;
    std::filesystem::path current_locker_file_; // A file with a path to locker_file_

    void change_locker_dir(
        std::filesystem::path dir, 
        bool same_ok = false, 
        bool is_storage_relative = true
    );

    void copy_locker(std::filesystem::path dest);
    void copy_token(std::filesystem::path dest);
    const std::filesystem::path get_locker_dir(bool is_full = true);

    void append_line_to_locker(std::string header, std::string content);
    void remove_line_from_locker(std::string header);
    std::string get_line_from_locker(std::string header);
    std::vector<std::string> get_all_lines_from_locker();

    std::string get_locker_salt();
    std::string get_token();
    std::string generate_token();
    bool token_exists();

private:
    void sync_locker();
};