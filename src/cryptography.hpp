#pragma once

#include "common.hpp"

#include <memory>
#include <string>
#include <array>

#define DERIVE_KEY_LENGTH 32
#define AES_IV_LENGTH     16 // AES initial vector

namespace crypt {
    using DerivedKey = std::array<uchar, DERIVE_KEY_LENGTH>;
};

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

    virtual std::string encrypt(const std::string& data) = 0;
    virtual std::string decrypt(const std::string& data) = 0;

    virtual bool cipher_initialized() {
        return key_set_;
    }

protected:
    int iterations_;
    bool key_set_ = false;
    crypt::DerivedKey key_;
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

    std::string encrypt(const std::string& data) override;
    std::string decrypt(const std::string& data) override;
};

class FernetBackend 
    : ACryptographyBackend 
{
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
    
    bool cipher_initialized();

private:
    std::unique_ptr<ACryptographyBackend> backend_; 
};