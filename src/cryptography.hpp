#pragma once

#include "common.hpp"

#include <memory>
#include <string>
#include <array>

#define FILE_CHUNK_SIZE   64 * 1024
#define DERIVE_KEY_SIZE   32

// version : timestamp : iv : ciphertext : hmac
//    1          8       16                 32

#define AES_IV_LENGTH            16 
#define FERNET_IV_LENGTH         16

#define FERNET_VERSION          0x80
#define FERNET_SKEY_SIZE        (DERIVE_KEY_SIZE / 2) // signing key 
#define FERNET_EKEY_SIZE        (DERIVE_KEY_SIZE / 2) // encryption key 
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

    virtual inline void xor_by_key(std::string& data) {
        for (uint i = 0; i < data.size(); i++) {
            data[i] ^= key_[i % key_.size()];
        }
    }

    virtual inline void xor_by_key(char* buf, uint n) {
        for (uint i = 0; i < n; i++) {
            buf[i] ^= key_[i % key_.size()];
        }
    }

protected:
    std::array<uchar, DERIVE_KEY_SIZE> key_;
    int iterations_;
    bool key_set_ = false;
};

class AESBackend 
    : public ACryptographyBackend 
{
public:
    AESBackend(int iterations)
        : ACryptographyBackend(iterations) {}

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
    FernetBackend(int iterations)
        : ACryptographyBackend(iterations) {}

    void init_cipher(
        const std::string& passphrase, 
        const std::string& salt, 
        const std::string& token
    ) override;

    std::string encrypt(std::string_view data) override;
    std::string decrypt(std::string_view data) override;

private:
    std::span<uchar> signing_key_;
    std::span<uchar> encryption_key_;

    std::vector<uchar> hmac(const std::vector<uchar>& data);
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

    std::unique_ptr<ACryptographyBackend> make_backend();
};