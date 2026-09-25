#pragma once

#include "filesystem.hpp"
#include "cryptography.hpp"

#include <functional>
#include <optional>
#include <unordered_map>
#include <thread>

struct EventHandler {
    std::function<void()> fn;
    bool is_async;
};

class EventManager 
{
public:
    ~EventManager();

    void subscribe(const std::string& event, 
        std::function<void()> fn, bool is_async);

    void trigger_event(const std::string& event);

    std::unordered_map<std::string,
        std::vector<EventHandler>> events_ = {};

private:
    std::vector<std::thread> pending_threads_ = {};
};

class Ceeper:
    public FileSystem,
    private CryptographySystem,
    public EventManager
{
public:
    Ceeper(
        uint token_size, uint salt_size, uint iterations,
        std::string backend, const char* program_name, bool is_portable = false
    ) 
    : FileSystem(salt_size, token_size, std::filesystem::path(program_name).parent_path(), is_portable), 
      CryptographySystem(iterations, backend)
    {
        trigger_event("init");
    }

    bool unlock(const std::string& passphrase);

    std::string generate_password(
        uint length, bool no_letters = false, bool no_special_syms = false 
    );
    
    std::vector<ceeper::Triplet> list_triplets();
    std::vector<ceeper::Triplet> search_for_triplets(const std::string& tag_part);
    std::optional<ceeper::Triplet> get_triplet(const std::string& tag);
    
    void store_triplet(ceeper::Triplet t);
    void remove_triplet(const std::string& tag);
    void edit_triplet(const std::string& tag, int property, std::string value);
    
    void file_encrypt(const std::string& passphrase, std::filesystem::path file, 
                      std::optional<std::filesystem::path> dest = std::nullopt);

    void file_decrypt(const std::string& passphrase, std::filesystem::path file, 
                      std::optional<std::filesystem::path> dest = std::nullopt);

    bool is_unlocked();

    std::string get_current_locker(bool is_full = true);
    
private:
    bool is_unlocked_ = false;
};