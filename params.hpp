#pragma once
namespace params {

/* 
* Number of iterations used to generate unique key.
* Key generated with '320000' iterations wont match a 
* key generated with '244444' iterations. 
* More iterations, more time it takes to unlock the locker.
* Optimal number of iterations is from 300000 to 400000
*/
const unsigned int ITERATIONS = 300000;

/*
 * Token size, will affect only new generated token. In case 
 * if actual token is longer than it's size, will use first amount of bytes
 * Optimal size: 32 to 64
 */
const unsigned int TOKEN_SIZE = 32;

/*
 * Same but size of the salt that's added at the beginning of each locker.
 * Warning! In case if locker's salt is less than number passed, will lead to
 * unexpected and fatal errors. Optimal size: 16 to 32 
 */
const unsigned int SALT_SIZE = 16;

/*
 * Encryption backend to use, each backend encrypts passwords in different way
 * Passwords encrypted with fernet backend wont be decrypted with AES one
 */
constexpr const char* BACKEND = "AES"; // or fernet

/*
 * Portable build. Will look for token, lokers in the 
 * same directory where the script is located.
 * Token should be in <keepr_dir>/data/
 * .lk files should be in <keeper_dir>/storage
 * Where keper_dir is the location where executable is located
 */
const bool IS_PORTABLE_BUILD = false;

// Extensions included in build. Leave list empty to disable any
constexpr const char* EXTENSIONS[] = { 
    "GitManager" 
};

} // namespace params