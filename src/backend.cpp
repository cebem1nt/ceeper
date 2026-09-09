#include "backend.hpp"
#include <random>

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
    
    for (const auto& line : encrypted_lines) {
        out.push_back(decrypt_triplet(line));
    }

    return out;
}

std::vector<Triplet> Ceeper::search_for_triplets(const std::string& tag_part) 
{
    auto encrypted_lines = get_all_lines_from_locker();
    auto part = to_lower(tag_part);
    std::vector<Triplet> out = {};
    
    for (const auto& line : encrypted_lines) {
        auto decrypted = decrypt_triplet(line);
        auto tag = to_lower(decrypted[0]);

        if (tag.find(part) != std::string::npos)
            out.push_back(decrypted);
    }

    return out;
}

std::optional<Triplet> Ceeper::get_triplet(const std::string& tag)
{
    auto hash_tag = hash(tag);
    auto found = get_line_from_locker(hash_tag);

    if (found.empty())
        return std::nullopt;

    return decrypt_triplet(found);
}

void Ceeper::store_triplet(ceeper::Triplet t) 
{
    auto encrypted = encrypt_triplet(t);
    append_line_to_locker(hash(t[0]), encrypted);
    // TODO events
}

void Ceeper::remove_triplet(const std::string& tag)
{
    remove_line_from_locker(hash(tag));
}

void Ceeper::edit_triplet(const std::string& tag, int property, std::string value) 
{
    auto to_edit = get_triplet(tag); 
    
    if (!to_edit)
        throw exc::NotFound("Could not find triplet!");

    (*to_edit)[property] = value;
    
    if (property == 0 && get_triplet(value))
        throw exc::AlreadyExists("Triplet with given tag already exists!");
    
    remove_triplet(tag);
    store_triplet(*to_edit);
}

std::string Ceeper::generate_password(uint length, bool no_letters, 
                                      bool no_special_syms) 
{
    const std::string letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string digits = "0123456789";
    const std::string special_symbols = R"(!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)";

    std::string chars;

    if (no_special_syms && no_letters)
        chars = digits;
    else if (no_special_syms)
        chars = letters + digits;
    else if (no_letters)
        chars = digits + special_symbols;
    else
        chars = letters + digits + special_symbols;

    if (chars.empty())
        throw exc::ValueMismatch("Character set is empty. Cannot generate a password!");

    std::random_device random;
    std::mt19937 gen(random());
    std::uniform_int_distribution<uint> dist(0, chars.size() - 1);

    std::string password;
    password.reserve(length);

    for (uint i = 0; i < length; i++) {
        password.push_back(chars[dist(gen)]);
    }

    return password;
}
    