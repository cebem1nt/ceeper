#pragma once

#include "common.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <array>

#define FILE_CHUNK_SIZE 64 * 1024
#define DERIVE_KEY_LENGTH 32

// Python fernet format:
// version : timestamp : iv : ciphertext : hmac
//    1          8       16                 32

#define FERNET_VERSION          0x80
#define FERNET_IV_LENGTH        16
#define FERNET_TIMESTAMP_SIZE   8
#define FERNET_HMAC_SIZE        32
#define FERNET_METAINFO_SIZE    \
    (1 + FERNET_TIMESTAMP_SIZE + FERNET_IV_LENGTH + FERNET_HMAC_SIZE)

#define AES_IV_LENGTH     16 // AES initial vector

class ACryptographyBackend 
{
public:
    explicit ACryptographyBackend(int iterations)
        : iterations_(iterations) {}

    virtual ~ACryptographyBackend() = default;    

    virtual void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    ) = 0;

    virtual std::string encrypt(std::string_view data) = 0;
    virtual std::string decrypt(std::string_view data) = 0;

    std::string encrypt(const std::string& data) {
        return encrypt(std::string_view(data));
    }

    std::string decrypt(const std::string& data) {
        return decrypt(std::string_view(data));
    }

    virtual bool cipher_initialized() {
        return key_set_;
    }

protected:
    int iterations_;
    bool key_set_ = false;
    std::array<uchar, DERIVE_KEY_LENGTH> key_;
};

class AESBackend 
    : public ACryptographyBackend 
{
public:
    AESBackend(int iterations):
        ACryptographyBackend(iterations) {}

    void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    ) override;

    std::string encrypt(std::string_view data) override;
    std::string decrypt(std::string_view data) override;
};

// https://github.com/Anthony-J-Garot/fernet_for_cpp/blob/main/fernet.cpp
class FernetBackend 
    : public ACryptographyBackend 
{
public:
    FernetBackend(int iterations):
        ACryptographyBackend(iterations) {}

    void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    ) override;

    std::string encrypt(std::string_view data) override;
    std::string decrypt(std::string_view data) override;

private:
    std::string encoded_key_;
    std::vector<uchar> signing_key_;
    std::vector<uchar> encryption_key_;
};

class CryptographySystem 
{
public:
    CryptographySystem(int iterations, std::string backend);

    void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    );
    
    std::string encrypt(const std::string& data);
    std::string decrypt(const std::string& data);
    std::string hash(const std::string& data); // SHA256
    
    std::string encrypt_triplet(ceeper::Triplet t);
    ceeper::Triplet decrypt_triplet(std::string data);
    
    void encrypt_file(const std::string& passphrase, const std::string& token, 
                      std::istream& src, std::ostream& dest);

    void decrypt_file(const std::string& passphrase, const std::string& token, 
                      std::istream& src, std::ostream& dest);

    bool cipher_initialized();

private:
    int iterations_;
    std::unique_ptr<ACryptographyBackend> backend_;
    std::string backend_name_;
};