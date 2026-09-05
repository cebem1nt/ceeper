#include <iostream>
#include "filesystem.hpp"

int main(int argc, char** argv)
{

    auto ff = FileSystem(16, 16, argv[0]);

    std::cout << ff.platform_ << "\n";
    std::cout << ff.data_dir_ << "\n";
    std::cout << ff.storage_dir_ << "\n";
    std::cout << ff.user_dir_ << "\n";

    std::cout << ff.get_locker_dir() << '\n';
    std::cout << ff.get_locker_dir(false) << '\n';
}