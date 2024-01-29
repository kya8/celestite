#include <cstring>

namespace clst::util {

// A stupid, spartan helper struct for manual cmdline argument parsing.
class ArgParse {
protected:
    bool err_flag = false; // set this to signal error
    bool stop = false;     // stop w/o error
    int i = 1;
    const int argc = 0;
    const char* const* const argv = nullptr;

    virtual void parse_current() = 0;
    virtual bool is_opt(const char*) { // is a recognized option,
        return false;
    }
    virtual bool check_next() noexcept {
        if (i + 1 >= argc || is_opt(argv[i + 1])) {
            err_flag = true;
            return false;
        }
        return true;
    }

    template<typename ...Ts>
    bool match_arg(Ts...args) const noexcept {
        return match_str(argv[i], args...);
    }

    template<typename ...Ts>
    bool match_str(const char* str, Ts...args) const noexcept {
        return ((!std::strcmp(str, args)) ||...);
    }

public:
    ArgParse(int argc, const char* const* argv) noexcept : argc(argc), argv(argv) {}
    bool parse() {
        while (i < argc) {
            if (err_flag || stop) break;
            parse_current();
            ++i;
        }
        return !err_flag;
    }
};

} // namespace clst::util
