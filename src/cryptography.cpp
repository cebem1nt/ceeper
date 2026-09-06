#include "cryptography.hpp"
#include "common.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <vector>

static crypt::key derive_key(
    const std::string& passphrase,
    const std::string& salt,
    const std::string& token,
    int iterations
)
{
    crypt::key derived_key;
    std::string combined_salt = token + salt;    
    
    PKCS5_PBKDF2_HMAC(
        passphrase.c_str(),
        passphrase.length(),
        RC<const uchar*>(combined_salt.c_str()),
        combined_salt.length(),
        iterations,
        EVP_sha256(),
        DERIVE_KEY_LENGTH,
        derived_key.data()
    );

    return derived_key;
}

static uint base_64url_decode_calc_length(const std::string& in) 
{
	int padding = 0;

    if (in.ends_with("=="))
        padding = 2;
    else if (in.ends_with("="))
        padding = 1;

	return (in.length()*3)/4 - padding;
}

// Accepts padded input only, as base64url_encode returns
static std::vector<uchar> base64url_decode(std::string in) 
{    
    // Convert back to standard base64    
    for (auto& c : in) {
        if (c == '-') 
            c = '+';
        else if (c == '_') 
            c = '/';
    }

    BIO* bio;
    BIO* b64;
    
    uint actual_len, calculated_len;
    calculated_len = base_64url_decode_calc_length(in);

    bio = BIO_new_mem_buf(in.data(), -1);
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    
    auto out = std::vector<uchar>(calculated_len);

    actual_len = BIO_read(bio, out.data(), out.size());
    BIO_free_all(bio);

    return out;
}


// https://gist.github.com/barrysteyn/7308212
static std::string base64url_encode(std::vector<uchar> in) 
{
    BIO* bio;
    BIO* b64;

    BUF_MEM* buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line

    BIO_write(bio, in.data(), in.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, buffer_ptr);

    auto out = std::string(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(bio);    

    // Convert standard base64 to base64url
    for (auto& c : out) {
        if (c == '+') 
            c = '-';
        else if (c == '/') 
            c = '_';
    }   
 
    return out;
}

AESBackend::AESBackend(int iterations)
{
    iterations_ = iterations;
}

void AESBackend::init_cipher(const std::string& passphrase, 
                             const std::string& salt, 
                             const std::string& token) 
{
    key_ = derive_key(passphrase, salt, token, iterations_);
}

std::string AESBackend::encrypt(const std::string& data) 
{
    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    EVP_CIPHER_CTX*   ctx = EVP_CIPHER_CTX_new();

    if (!ctx)
        FATAL("AESBackend: EVP_CIPHER_CTX_new failed");

    const auto* input = RC<const uchar*>(data.data());
    
    auto ciphertext = std::vector<uchar> (
        data.size() + EVP_CIPHER_block_size(cipher)
    );
    
    int update_len = 0, final_len = 0, ciphertext_len = 0;

    std::array<uchar, AES_IV_LENGTH> iv;

    if (RAND_bytes(iv.data(), AES_IV_LENGTH) != 1)
        FATAL("AESBackend: RAND_bytes failed");

    if (EVP_EncryptInit(ctx, cipher, key_.data(), iv.data()) != 1)
        FATAL("AESBackend: EVP_EncryptInit failed");   

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &update_len, input, data.size()) != 1) 
        FATAL("AESBackend: EVP_EncryptUpdate failed");

    if (EVP_EncryptFinal(ctx, ciphertext.data() + update_len, &final_len) != 1)
        FATAL("AESBackend: EVP_EncryptFinal failed");

    EVP_CIPHER_CTX_free(ctx);

    ciphertext_len = update_len + final_len;
    auto out = std::vector<uchar>(iv.begin(), iv.end());

    out.insert(out.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);

    return base64url_encode(out);
}

std::string AESBackend::decrypt(const std::string& data) 
{
    const auto encryped = base64url_decode(data);
    
    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    EVP_CIPHER_CTX*   ctx = EVP_CIPHER_CTX_new();
    
    const uchar* iv = encryped.data();
    const uchar* ciphertext = encryped.data() + AES_IV_LENGTH;
    int   ciphertext_len = encryped.size() - AES_IV_LENGTH;
    int   update_len = 0, final_len = 0; 

    auto plaintext = std::vector<uchar>(
            encryped.size() + EVP_CIPHER_block_size(cipher));
    
    if (!ctx)
        FATAL("AESBackend: EVP_CIPHER_CTX_new failed");

    if (EVP_DecryptInit(ctx, cipher, key_.data(), iv) != 1)
        FATAL("AESBackend: EVP_DecryptInit_ex failed");

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &update_len, ciphertext, ciphertext_len) != 1)
        FATAL("AESBackend: EVP_DecryptUpdate failed");

    if (EVP_DecryptFinal(ctx, plaintext.data() + update_len, &final_len) != 1)
        FATAL("AESBackend: invalid ciphertext or padding");

        EVP_CIPHER_CTX_free(ctx);


    return std::string(
        RC<char*>(plaintext.data()),
        update_len + final_len
    );
} 


CryptographySystem::CryptographySystem(int iterations, std::string backend) 
{

}