#include "backend.hpp"

using namespace ceeper;

bool Ceeper::unlock(const std::string& passphrase) 
{
    auto token = get_token();
    auto salt = get_locker_salt();

    if (token.empty())
        throw exc::NotInitialized("Token was not generated!");

    try {
        init_cipher(passphrase, salt, token);
        list_triplets();
        return true;
    } catch (const exc::DecryptionError& e) { // Could not decrypt -> mismatching key
        return false;
    }
}

std::vector<Triplet> Ceeper::list_triplets() 
{
    auto encrypted_lines = get_all_lines_from_locker();
    std::vector<Triplet> out = {};
    
    for (auto& line : encrypted_lines) {
        out.push_back(decrypt_triplet(line));
    }

    return out;
}
