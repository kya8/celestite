#include "test.h"

// Disabled for now

// Exercise compile-time stripping and runtime filtering in the same test.
#ifdef CLST_LOG_LEVEL
#undef CLST_LOG_LEVEL
#endif
#define CLST_LOG_LEVEL CLST_LOG_LEVEL_DEBUG
#include <clst/log.hpp>
#include <chrono>
#include <memory>
#include <utility>

namespace {

namespace logging = clst::log;
using logging::Level;

auto make_log_file(std::string_view tag)
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto path = std::format("clst-logger-{}-{}.tmp", stamp, tag);
#ifdef _MSC_VER
    std::FILE* file = nullptr;
    CLST_ASSERT_EQ(::fopen_s(&file, path.c_str(), "w+bx"), 0);
#else
    auto* file = std::fopen(path.c_str(), "w+bx");
#endif
    CLST_ASSERT(file);

    auto cleanup = [path = std::move(path)](std::FILE* stream) {
        std::fclose(stream);
        std::remove(path.c_str());
    };
    return std::unique_ptr<std::FILE, decltype(cleanup)>{file, std::move(cleanup)};
}

void expect_line(std::FILE* file, std::string_view body)
{
    char buffer[128];
    CLST_ASSERT(std::fgets(buffer, sizeof(buffer), file));
    const std::string_view line(buffer);
    CLST_ASSERT_EQ(line.size(), 13 + body.size());
    CLST_ASSERT(line[2] == ':' && line[5] == ':' && line[8] == '.' && line[12] == ' ');
    CLST_ASSERT_EQ(line.substr(13), body);
}

} // namespace

int logger(int, char*[])
{
    CLST_ASSERT_EQ(logging::sinks().size(), 1u);
    const auto default_sink = logging::sinks().front();
    CLST_ASSERT_EQ(default_sink.file, stdout);
    CLST_ASSERT_EQ(default_sink.level, Level::info);

    logging::add_sink({.file = stdout, .level = Level::off});
    CLST_ASSERT_EQ(logging::sinks().size(), 1u);
    CLST_ASSERT_EQ(logging::sinks().front().level, Level::off);

    const auto plain = make_log_file("plain");
    const auto colored = make_log_file("colored");
    logging::add_sink({.file = plain.get()});
    logging::add_sink({.file = colored.get(), .level = Level::warn, .color = true});

    CLST_LOG_TRACE("{}", compiled_out_argument);
    int evaluations = 0;
    CLST_LOG_DEBUG("filtered {}", ++evaluations);
    CLST_ASSERT_EQ(evaluations, 0);
    CLST_LOG_INFO("value {}", 42);

    for (auto& sink : logging::sinks()) {
        if (sink.file == plain.get()) {
            sink.level = Level::debug;
        }
    }
    CLST_LOG_DEBUG("value {}", ++evaluations);
    CLST_ASSERT_EQ(evaluations, 1);
    logging::write(Level::off, "filtered");
    CLST_LOG_WARN("warning");

    logging::remove_sink(plain.get());
    CLST_LOG_WARN("after removal");
    logging::remove_sink(colored.get());
    logging::add_sink(default_sink);

    CLST_ASSERT_EQ(std::fseek(plain.get(), 0, SEEK_SET), 0);
    expect_line(plain.get(), "[info] value 42\n");
    expect_line(plain.get(), "[debug] value 1\n");
    expect_line(plain.get(), "[warn] warning\n");
    CLST_ASSERT_EQ(std::fgetc(plain.get()), EOF);

    CLST_ASSERT_EQ(std::fseek(colored.get(), 0, SEEK_SET), 0);
    expect_line(colored.get(), "[\x1b[33mwarn\x1b[0m] warning\n");
    expect_line(colored.get(), "[\x1b[33mwarn\x1b[0m] after removal\n");
    CLST_ASSERT_EQ(std::fgetc(colored.get()), EOF);
    return 0;
}
