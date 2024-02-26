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

    virtual void parseCurrent() = 0;
    virtual bool isOpt(const char*) { // is a recognized option,
        return false;
    }
    virtual bool checkNext() noexcept {
        if (i + 1 >= argc || isOpt(argv[i + 1])) {
            err_flag = true;
            return false;
        }
        return true;
    }

    template<typename ...Ts>
    bool matchArg(Ts...args) const noexcept {
        return matchStr(argv[i], args...);
    }

    template<typename ...Ts>
    bool matchStr(const char* str, Ts...args) const noexcept {
        return ((!std::strcmp(str, args)) ||...);
    }

public:
    ArgParse(int argc, const char* const* argv) noexcept : argc(argc), argv(argv) {}
    bool parse() {
        while (i < argc) {
            if (err_flag || stop) break;
            parseCurrent();
            ++i;
        }
        return !err_flag;
    }
};

} // namespace clst::util
