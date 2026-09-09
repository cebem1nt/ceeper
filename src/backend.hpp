#pragma once

#include "filesystem.hpp"
#include "cryptography.hpp"

#include <optional>

class Ceeper:
    private FileSystem,
    private CryptographySystem
{
public:
    Ceeper(
        uint token_size, uint salt_size, uint iterations,
        std::string backend, const char* program_name, bool is_portable = false
    ) 
    : FileSystem(salt_size, token_size, std::filesystem::path(program_name).parent_path(), is_portable), 
      CryptographySystem(iterations, backend)
    {}

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
};