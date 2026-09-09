#include "backend.hpp"

class CLI 
{
public:
    CLI(Ceeper& instance) :
        ceeper_(instance) {};

    void welcome();
    void auth();
    void registrate();
    void login();

private:
    Ceeper& ceeper_;
};