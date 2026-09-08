#include <iostream>

#include "filesystem.hpp"
#include "cryptography.hpp"

int main(int argc, char** argv)
{

    auto ff = FileSystem(16, 16, argv[0]);

    std::cout << ff.platform_ << "\n";
    std::cout << ff.data_dir_ << "\n";
    std::cout << ff.storage_dir_ << "\n";
    std::cout << ff.user_dir_ << "\n";

    std::cout << ff.get_locker_dir() << '\n';
    std::cout << ff.get_locker_dir(false) << '\n';

    auto cs = CryptographySystem(235, "AES");
    
    std::cout << cs.cipher_initialized() << "\n";

    std::string key(32, 'a');
    std::string a = "aaaaaaaaaaaaaaa";

    cs.init_cipher(key, key, key);

    std::cout << cs.cipher_initialized() << '\n';

    auto encrypted = cs.encrypt(a);
    std::cout << encrypted << '\n';

    auto decrypted = cs.decrypt(encrypted);
    std::cout << decrypted << '\n';
}