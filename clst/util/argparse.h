#include <cstring>
#include <vector>

namespace clst::util {

// A stupid, spartan helper class for command line argument parsing.
// TODO: templated conversion methods.
class ArgParse {
protected:
    bool err_flag = false; // set this to signal error
    int i = 1;
    const int argc = 0;
    char** const argv = nullptr;

    virtual void parse_current() = 0;
    virtual bool check_next() noexcept {
        if (i + 1 >= argc || argv[i + 1][0] == '-') {
            err_flag = true;
            return false;
        }
        return true;
    }
    
    template<typename ...Ts>
    bool match_arg(Ts...args) const noexcept {
        return ((!std::strcmp(argv[i], args)) ||...);
    }

public:
    ArgParse(int argc, char** argv) noexcept : argc(argc), argv(argv) {}
    bool parse() {
        while (i < argc) {
            if (err_flag) break;
            parse_current();
            ++i;
        }
        return !err_flag;
    }
    std::vector<const char*> positional_args;
};

} // namespace clst::util

#if 0
// an example
class MyArgs final : public ArgParse {
public:
    std::string arg_long, arg_x;
    bool has_s = 0;
    bool has_long = 0;

    using ArgParse::ArgParse;

protected:
    virtual void parse_current() override {
        if (match_arg("--long_arg")) {
            if (check_next()) {
                arg_long = argv[++i];
            }
        }
        else if (match_arg("-x", "--longx")) {
            if (check_next()) {
                arg_x = argv[++i];
            }
        }
        else if (match_arg("-s")) {
            has_s = 1;
        }
        else if (match_arg("--long")) {
            has_long = 1;
        }
        else {
            positional_args.push_back(argv[i]);
        }
    }
};
#endif
