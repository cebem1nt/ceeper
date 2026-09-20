#include "backend.hpp"
#include "common.hpp"

class AExtension {
public:
    explicit AExtension(Ceeper& ceeper):
        ceeper_(ceeper) {}

    virtual ~AExtension() = default;

    virtual void subscribe() {
        throw exc::NotImplemented("AExtension: subscribe not implemented");
    }

public:
    Ceeper& ceeper_;
};

class GitManager
    : public AExtension 
{
public:
    GitManager(Ceeper& ceeper);

    virtual void subscribe() override;

private:
    std::string git_run(const std::string& cmd);

    template <typename... Args>
    std::string git(std::format_string<Args...> prompt, Args&&... args) {
        auto formatted = std::format(prompt, std::forward<Args>(args)...);
        return git_run(formatted);
    }

    void init_repo();
    void pull();
    void push();

    std::filesystem::path storage_dir_;
    std::filesystem::path git_dir_;
};