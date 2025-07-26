#ifndef CLST_DSO_HPP
#define CLST_DSO_HPP

namespace clst {

class Dso {
public:
    Dso() noexcept = default;
    Dso(const char* name) noexcept;
    ~Dso() noexcept;
    // Can't copy ourselves.
    Dso(const Dso&) = delete;
    Dso& operator=(const Dso&) = delete;

    Dso(Dso&&) noexcept;
    Dso& operator=(Dso&&) noexcept;

    bool open(const char* name) noexcept;
    bool is_open() noexcept;
    bool close() noexcept;
    void* get_sym(const char* name) const noexcept;
    auto get_handle() noexcept {
        return handle_;
    }

private:
    void* handle_ = nullptr;
};

} // namespace clst

#endif // CLST_DSO_HPP
