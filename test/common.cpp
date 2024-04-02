#include <clst/version.h>
#include <gtest/gtest.h>
#include <cstring>

static bool checkVersion (const char* s) {
    return (s && std::strcmp(s, "unknown") && std::strcmp(s, "Unknown"));
}

TEST(Common, Version) {
    using namespace clst::version;
    for (const auto version_string : {GIT_DESC, GIT_BRANCH, COMMIT_DATE, COMMIT_HASH, TARGET_OS, TARGET_ARCH, COMPILER_NAME, COMPILER_VERSION, HOST_OS}) {
        EXPECT_TRUE(checkVersion(version_string));
    }
}
