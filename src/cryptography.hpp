#pragma once

#include "common.hpp"

#include <memory>
#include <string>
#include <array>

#define DERIVE_KEY_LENGTH 32
#define AES_IV_LENGTH     16 // AES initial vector

// Python fernet format:
// version : timestamp : iv : ciphertext : hmac
//    1          8       16                 32

#define FERNET_VERSION          0x80
#define FERNET_IV_LENGTH        16
#define FERNET_TIMESTAMP_SIZE   8
#define FERNET_HMAC_SIZE        32
#define FERNET_METAINFO_SIZE    \
    (1 + FERNET_TIMESTAMP_SIZE + FERNET_IV_LENGTH + FERNET_HMAC_SIZE)

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

    std::string encrypt(const std::string& data) override;
    std::string decrypt(const std::string& data) override;
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

    std::string encrypt(const std::string& data) override;
    std::string decrypt(const std::string& data) override;

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
    
    bool cipher_initialized();

private:
    std::unique_ptr<ACryptographyBackend> backend_; 
};