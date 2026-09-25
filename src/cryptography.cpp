#include "cryptography.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include <format>
#include <iomanip>
#include <algorithm>
#include <regex>
#include <vector>

namespace fs = std::filesystem;

static std::array<uchar, DERIVE_KEY_LENGTH> derive_key(
    const std::string& passphrase,
    const std::string& salt,
    const std::string& token,
    int iterations
)
{
    std::array<uchar, DERIVE_KEY_LENGTH> derived_key;
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

static std::vector<uchar> hmac_sha256(const std::vector<uchar>& key,
                                        const std::vector<uchar>& data)
{
    std::vector<uchar> out(EVP_MAX_MD_SIZE);
    uint len = 0;

    HMAC(EVP_sha256(),
        key.data(), static_cast<int>(key.size()),
        data.data(), data.size(),
        out.data(), &len);

    out.resize(len);
    return out;
}

static bool is_equal(const std::vector<uchar>& a,
                     const std::vector<uchar>& b)
{
    if (a.size() != b.size())
        return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
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
static std::vector<uchar> base64_decode(const std::string& in) 
{
    BIO* bio;
    BIO* b64;
    
    uint calculated_len = base_64url_decode_calc_length(in);

    bio = BIO_new_mem_buf(in.data(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer
    
    auto out = std::vector<uchar>(calculated_len);

    BIO_read(bio, out.data(), out.size());
    BIO_free_all(bio);

    return out;
}


// https://gist.github.com/barrysteyn/7308212
static std::string base64_encode(const std::vector<uchar>& in) 
{
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());

    BUF_MEM* buffer_ptr = nullptr;
	bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Ignore newlines - write everything in one line

    BIO_write(bio, in.data(), in.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);

    auto out = std::string(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(bio);    
 
    return out;
}

static std::vector<uchar> base64url_decode(std::string in) 
{
    // Convert back to standard base64    
    for (auto& c : in) {
        if (c == '-') 
            c = '+';
        else if (c == '_') 
            c = '/';
    }

    return base64_decode(in);
}

static std::string base64url_encode(std::vector<uchar> in) 
{
    auto out = base64_encode(in);
    
    // Convert standard base64 to base64url
    for (auto& c : out) {
        if (c == '+') 
            c = '-';
        else if (c == '/') 
            c = '_';
    }   

    return out;
}

void AESBackend::init_cipher(const std::string& passphrase, 
                             const std::string& salt, 
                             const std::string& token) 
{
    key_ = derive_key(passphrase, salt, token, iterations_);
    key_set_ = true;
}

std::string AESBackend::encrypt(std::string_view data) 
{
    if (!key_set_)
        throw exc::NotInitialized("Key was not initialized!");

    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    EVP_CIPHER_CTX*   ctx = EVP_CIPHER_CTX_new();

    if (!ctx)
        FATAL("AESBackend: EVP_CIPHER_CTX_new failed");

    const auto* input = RC<const uchar*>(data.data());
    const auto  iv    = urandom(AES_IV_LENGTH);
    auto ciphertext   = std::vector<uchar>(data.size() + EVP_CIPHER_block_size(cipher));
    
    int update_len = 0, final_len = 0, ciphertext_len = 0;

    if (EVP_EncryptInit(ctx, cipher, key_.data(), iv.data()) != 1)
        throw exc::EncryptionError("AESBackend: EVP_EncryptInit failed");   

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &update_len, input, data.size()) != 1) 
        throw exc::EncryptionError("AESBackend: EVP_EncryptUpdate failed");

    if (EVP_EncryptFinal(ctx, ciphertext.data() + update_len, &final_len) != 1)
        throw exc::EncryptionError("AESBackend: EVP_EncryptFinal failed");

    EVP_CIPHER_CTX_free(ctx);

    ciphertext_len = update_len + final_len;
    auto out = std::vector<uchar>(iv.begin(), iv.end());

    out.insert(out.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);

    return base64url_encode(out);
}

std::string AESBackend::decrypt(std::string_view data) 
{
    if (!key_set_)
        throw exc::NotInitialized("Key was not initialized!");
    
    const EVP_CIPHER* cipher = EVP_aes_256_cbc();
    EVP_CIPHER_CTX*   ctx    = EVP_CIPHER_CTX_new();

    if (!ctx)
        FATAL("AESBackend: EVP_CIPHER_CTX_new failed");

    const auto   encrypted  = base64url_decode(std::string(data));
    const uchar* iv         = encrypted.data();
    const uchar* ciphertext = encrypted.data() + AES_IV_LENGTH;
    
    int update_len = 0, final_len = 0,
        ciphertext_len = encrypted.size() - AES_IV_LENGTH;

    auto plaintext = std::vector<uchar>(
        ciphertext_len + EVP_CIPHER_block_size(cipher));

    if (EVP_DecryptInit(ctx, cipher, key_.data(), iv) != 1)
        throw exc::DecryptionError("AESBackend: EVP_DecryptInit failed!");

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &update_len, ciphertext, ciphertext_len) != 1)
        throw exc::DecryptionError("AESBackend: EVP_DecryptUpdate failed");

    if (EVP_DecryptFinal(ctx, plaintext.data() + update_len, &final_len) != 1)
        throw exc::DecryptionError("AESBackend: invalid ciphertext or padding");

    EVP_CIPHER_CTX_free(ctx);

    return std::string(
        RC<char*>(plaintext.data()),
        update_len + final_len
    );
}

void FernetBackend::init_cipher(const std::string& passphrase, 
                                const std::string& salt, 
                                const std::string& token) 
{
    key_ = derive_key(passphrase, salt, token, iterations_);
    signing_key_ = std::vector<uchar>(key_.begin(), key_.begin() + 16);
    encryption_key_ = std::vector<uchar> (key_.begin() + 16, key_.end());
    key_set_ = true;
}

std::string FernetBackend::encrypt(std::string_view data)
{
    if (!key_set_)
        throw exc::NotInitialized("Cipher wasn't initialized");

    const EVP_CIPHER* cipher = EVP_aes_128_cbc();
    EVP_CIPHER_CTX*   ctx    = EVP_CIPHER_CTX_new();

    if (!ctx)
        FATAL("FernetBackend: EVP_CIPHER_CTX_new failed");
    
    const auto* input = RC<const uchar*>(data.data());
    const auto  iv    = urandom(FERNET_IV_LENGTH);
    auto ciphertext   = std::vector<uchar>(data.size() + EVP_CIPHER_block_size(cipher));

    int update_len = 0, final_len = 0;

    if (EVP_EncryptInit(ctx, cipher, encryption_key_.data(), iv.data()) != 1)
        throw exc::EncryptionError("FernetBackend: EVP_EncryptInit failed");

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &update_len, input, data.size()) != 1)
        throw exc::EncryptionError("FernetBackend: EVP_EncryptUpdate failed");
    
    if (EVP_EncryptFinal(ctx, ciphertext.data() + update_len, &final_len) != 1)
        throw exc::EncryptionError("FernetBackend: EVP_EncryptFinal failed");

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(update_len + final_len);

    std::vector<uchar> token;
    token.reserve(ciphertext.size() + FERNET_METAINFO_SIZE);
    token.push_back(FERNET_VERSION);

    uint64_t ts = SC<uint64_t>(std::time(nullptr));

    for (int i = 7; i >= 0; i--) {
        token.push_back(SC<uchar>((ts >> (i * 8)) & 0xFF));
    }

    token.insert(token.end(), iv.begin(), iv.end());
    token.insert(token.end(), ciphertext.begin(), ciphertext.end());

    const auto mac = hmac_sha256(signing_key_, token);
    token.insert(token.end(), mac.begin(), mac.end());

    return base64url_encode(token);
}

std::string FernetBackend::decrypt(std::string_view data) 
{
    if (!key_set_)
        throw exc::NotInitialized("Cipher wasn't initialized");

    auto token = base64url_decode(std::string(data));

    if (token.size() < FERNET_METAINFO_SIZE)
        throw exc::DecryptionError("Token doesn't match");

    if (token[0] != FERNET_VERSION)
        throw exc::DecryptionError("FernetBackend: invalid version");

    auto signed_part  = std::vector<uchar>(token.begin(), token.end() - FERNET_HMAC_SIZE);
    auto expected_mac = std::vector<uchar>(token.end() - FERNET_HMAC_SIZE, token.end());
    auto actual_mac   = hmac_sha256(signing_key_, signed_part);

    if (!is_equal(expected_mac, actual_mac))
        throw exc::DecryptionError("Token doesn't match");

    const EVP_CIPHER* cipher = EVP_aes_128_cbc();
    EVP_CIPHER_CTX*   ctx    = EVP_CIPHER_CTX_new();

    if (!ctx)
        FATAL("FernetBackend: EVP_CIPHER_CTX_new failed");
    
    const uchar* iv         = token.data() + 1 + FERNET_TIMESTAMP_SIZE;
    const uchar* ciphertext = token.data() + 1 + FERNET_TIMESTAMP_SIZE + FERNET_IV_LENGTH;

    int update_len = 0, final_len = 0, 
        ciphertext_len = SC<int>(token.size() - FERNET_METAINFO_SIZE);

    auto plaintext = std::vector<uchar>(
        ciphertext_len + EVP_CIPHER_block_size(cipher));

    if (EVP_DecryptInit(ctx, cipher, encryption_key_.data(), iv) != 1)
        throw exc::DecryptionError("FernetBackend: EVP_DecryptInit failed");

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &update_len, ciphertext, ciphertext_len) != 1)
        throw exc::DecryptionError("FernetBackend: EVP_DecryptUpdate failed");

    if (EVP_DecryptFinal(ctx, plaintext.data() + update_len, &final_len) != 1)
        throw exc::DecryptionError("FernetBackend: invalid ciphertext or padding");

    EVP_CIPHER_CTX_free(ctx);

    return std::string(
        RC<char*>(plaintext.data()), 
        update_len + final_len
    );
}

CryptographySystem::CryptographySystem(int iterations, std::string backend) 
{
    std::transform(backend.begin(), backend.end(), backend.data(), ::tolower);
    
    backend_name_ = backend;
    iterations_ = iterations;

    if (backend == "aes")
        backend_ = std::make_unique<AESBackend>(iterations);   
    else
        backend_ = std::make_unique<FernetBackend>(iterations);
}

void CryptographySystem::init_cipher(
    const std::string& passphrase, const std::string& salt, const std::string& token) 
{
    return backend_->init_cipher(passphrase, salt, token);
}

bool CryptographySystem::cipher_initialized() 
{
    return backend_->cipher_initialized();
}

std::string CryptographySystem::encrypt(const std::string& data) 
{
    return backend_->encrypt(data);
}

std::string CryptographySystem::decrypt(const std::string& data) 
{
    return backend_->decrypt(data);
}

std::string CryptographySystem::hash(const std::string& data)
{
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    uchar       hash[EVP_MAX_MD_SIZE];
    uint        hash_length = 0;

    if (!ctx)
        FATAL("CryptographySystem::hash EVP_CIPHER_CTX_new failed");

    const bool success = 
        EVP_DigestInit(ctx, EVP_sha256()) == 1 &&
        EVP_DigestUpdate(ctx, data.data(), data.size()) == 1 &&
        EVP_DigestFinal(ctx, hash, &hash_length) == 1;

    EVP_MD_CTX_free(ctx);

    if (!success)
        FATAL("CryptographySystem::hash failed to compute sha256");

    std::ostringstream out;
    out << std::hex << std::setfill('0');

    for (uint i = 0; i < hash_length; i++) {
        out << std::setw(2) << SC<uint>(hash[i]);
    }

    return out.str();
}

std::string CryptographySystem::encrypt_triplet(ceeper::Triplet t) 
{
    auto escape_brackets = [](std::string in) {
        in = std::regex_replace(in, std::regex(R"(\[)"), R"(\[)");
        in = std::regex_replace(in, std::regex(R"(\])"), R"(\])");
        return in;
    };

    auto [tag, login, password] = t;
    
    tag = escape_brackets(tag);
    login = escape_brackets(login);
    password = escape_brackets(password);
 
    auto formatted = std::format("[ {} ] [ {} ] [ {} ]",
        tag, login, password);

    return encrypt(formatted);
}

ceeper::Triplet CryptographySystem::decrypt_triplet(std::string data) 
{
    auto restore_brackets = [](std::string in) {
        in = std::regex_replace(in, std::regex(R"(\\\[)"), "[");
        in = std::regex_replace(in, std::regex(R"(\\\])"), "]");
        return in;
    };

    const auto decrypted = decrypt(data);
    const std::regex pattern(R"(\[\s*((?:\\.|[^\]])*?)\s*\])");

    std::sregex_iterator it(decrypted.begin(), decrypted.end(), pattern);
    std::sregex_iterator end;

    ceeper::Triplet out;
    std::size_t count = 0;

    for (; it != end; it++) {
        if (count == out.size())
            throw exc::FileMalformed("Malformed .lk file!");

        out[count++] = restore_brackets((*it)[1].str());
    }

    if (count != out.size())
        throw exc::FileMalformed("Malformed .lk file!");

    return out;
}

void CryptographySystem::encrypt_file(const std::string& passphrase, const std::string& token,
                                      std::istream& src, std::ostream& dest)
{
    auto salt = surandom(16);

    // We need a tmp backend here to not mess up original one
    std::unique_ptr<ACryptographyBackend> tmpb;

    if (backend_name_ == "aes")
        tmpb = std::make_unique<AESBackend>(iterations_);   
    else
        tmpb = std::make_unique<FernetBackend>(iterations_);

    tmpb->init_cipher(passphrase, salt, token);
    auto buf = std::array<char, FILE_CHUNK_SIZE>();
    dest << salt;
    
    while (src) {
        src.read(buf.data(), buf.size());
        auto n = src.gcount();
        if (n <= 0) 
            break;

        std::string_view chunk(buf.data(), n);
        auto encrypted_chunk = tmpb->encrypt(chunk);

        dest << encrypted_chunk << '\n';
        dest.flush();
    }

    // TODO we can xor it on top by token
}

void CryptographySystem::decrypt_file(const std::string& passphrase, const std::string& token,
                                      std::istream& src, std::ostream& dest)
{
    std::unique_ptr<ACryptographyBackend> tmpb;

    if (backend_name_ == "aes")
        tmpb = std::make_unique<AESBackend>(iterations_);   
    else
        tmpb = std::make_unique<FernetBackend>(iterations_);

    auto salt = std::string(16, '\0');   
    src.read(salt.data(), 16);

    tmpb->init_cipher(passphrase, salt, token);
    std::string line;

    while (std::getline(src, line)) {
        auto original = tmpb->decrypt(line);
        dest << original;
    }        
}