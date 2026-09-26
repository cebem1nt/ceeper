# Ceeper

A local cli password manager writen in c++23 with git syncronization. A fully compatible rewrite of [keeper](https://github.com/cebem1nt/keeper) with additional features

## Installation

### Tweaking

Before building, you can tweak default cryptography params, used extensions and other things by modifying `params.hpp`:

```cpp
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
```

### Build

#### Posix:

```sh
make && sudo make install
```

#### Windows (MinGW/MSYS2 shell):

```sh
make
```

## Usage

Firstly generate a token - unique _pepper_ for your .lk files stored on your local machine. If you're going to use ceeper on multiple devices, each device must have the same token.

Generate with:

```sh
cee --generate-token
```

It will then be stored at `~/.local/share/keeper/token`

### Examples

Add a triplet (tag/login/password) with `google` tag

```sh
cee add google # also cee a google
```

Get triplet with `google` tag

```sh
cee get google # also cee g google
```

List all triplets and show their passwords 

```sh
cee list -s # also cee ls -s
```

Remove triplet with `google` tag

```sh
cee remove google # also cee rm google
```

Add a triplet with tag `github` and a 32 chars generated password 

```sh
cee a github -g -l 32
```

Generate a password and print it to stdout instead of copying to clipboard

```sh
cee -g -p
```

Get current locker absolute file path

```sh
cee -c -a
```

Encrypt a file in place

```sh
cee -e secrets.txt -i
```

Decrypt file and store it as `not_secrets.txt`, remove encrypted file

```sh
cee -d secrets.enc -o not_secrets.txt -i
```

Interactive / REPL mode

```sh
cee
```

### List of commands

```
Usage: cee [--help] [--current] [--force] [--absolute] [--print] [--gen] [--no-letters] [--no-symbols] [--length VAR] [--encrypt FILE] [--decrypt FILE] [--out OUT] [--in-place] [--generate-token] {add,change,edit,find,get,list,remove}

Subcommands:
  add                add a new triplet with given TAG
  change             changes current locker file to LOCKER
  edit               interactively edit triplet[s] by TAG[s]
  find               look for triplets by given tag PART[s]
  get                get password by TAG
  list               list triplets
  remove             remove triplet[s] by TAG[s]

Optional arguments:
  -h, --help          shows help message and exits
  -c, --current       print current locker path
  -f, --force         force action, don't prompt for confirmation
  -a, --absolute      treat locker paths as non relative to storage dir
  -p, --print         print to stdout instead of copying
  -g, --gen           generate a password, copy, and store it with tag
  -nl, --no-letters   generate a password without any letters.
  -ns, --no-symbols   generate a password without any special symbols.
  -l, --length        length for newly generated password [default: 16]
  -e, --encrypt FILE  prompts for password, encrypts given file
  -d, --decrypt FILE  prompts for password, decrypts given file
  -o, --out OUT       exact out file when using -e/-d
  -i, --in-place      after encrypting / decrypting, remove original file
  --generate-token    generate a new token

Each password file is referred to as a "locker" and has a .lk extension.
You can manage multiple lockers, each containing different sets of passwords.
Passwords are stored in a triplet format: tag/login/password.
Use the tag to retrieve detailed information about each triplet.
Lockers are encrypted with a passphrase and your unique token.
If you want to use your lockers on multiple devices, you need the same token.
```

## Notes

### .lk files

Files in which passwords are stored. format is the following:

```
[nbytes salt][sha256 of a tag][encrypted]\n
[sha256 of a tag][encrypted triplet]\n
[sha256 of a tag][encrypted triplet]\n
```

### Storage

You can find your .lk files at:

- `~/.keeper_storage` (linux, mac, termux)
- `C:\Users\<Username>\.keeper_storage` (windows)

Or alternatively ***you can set custom directory with*** `$KEEPER_STORAGE_DIR` environment variable, for example:

`export KEEPER_STORAGE_DIR="$XDG_STATE_HOME/keeper_storage"`

### Git synchronization

Available if compiled with `GitManager` extension. 

Ensure all of your devices have the same token, otherwise, ***things won't decrypt***

You will be prompted to setup a git repo if not found, altenatively setup a git repo in you storage_dir, after that everything will be managed automatically.

## TODO

- [x] Crossplatform build
- [ ] Windows support
