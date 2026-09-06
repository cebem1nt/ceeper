#pragma once

#include <memory>
#include <string>
#include <array>

#define DERIVE_KEY_LENGTH 32
#define AES_IV_LENGTH     16 // AES initial vector

namespace crypt {
    // 0 - Tag, 1 - Login, 2 - Password
    using triplet = std::array<std::string, 3>;
    using key = std::array<unsigned char, DERIVE_KEY_LENGTH>;
};


class ACryptographyBackend 
{
public:
    virtual void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    ) = 0;

    virtual std::string encrypt(const std::string& data) = 0;
    virtual std::string decrypt(const std::string& data) = 0;
    virtual bool cipher_initialized() = 0;
};

class AESBackend : public ACryptographyBackend 
{
    AESBackend(int iterations);

    void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    ) override;

    std::string encrypt(const std::string& data) override;
    std::string decrypt(const std::string& data) override;

private:
    int iterations_;
    crypt::key key_;
};

class FernetBackend : ACryptographyBackend 
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
    
    std::string encrypt(std::string data);
    std::string decrypt(std::string data);
    std::string hash(std::string data);    
    
    std::string encrypt_triplet(crypt::triplet t);
    crypt::triplet decrypt_triplet(std::string data);
    
    bool cipher_initialized();

private:
    std::unique_ptr<ACryptographyBackend> backend_; 
};