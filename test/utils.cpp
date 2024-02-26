#include <clst/util/argparse.h>
#include <clst/util/char_num.h>
#include <gtest/gtest.h>
#include <vector>
#include <string>

using namespace clst::util;

TEST(ArgParse, BasicWithNum)
{
    struct MyArgs : ArgParse {
        using ArgParse::ArgParse;

        double long_arg;
        int    short_arg;
        bool   has_v = false;
        std::vector<std::string> positional_args;

        void parseCurrent() override {
            if (matchArg("-v", "--v_long")) {
                has_v = true;
            }
            else if (matchArg("--long")) {
                if(checkNext()) {
                    if(!char_num(argv[++i], long_arg)) {
                        err_flag = true;
                    }
                }
            }
            else if (matchArg("-s")) {
                if(checkNext()) {
                    if(!char_num(argv[++i], short_arg)) {
                        err_flag = true;
                    }
                }
            }
            else {
                positional_args.emplace_back(argv[i]);
            }
        }
    };

    const std::vector args{"foobaz", "--long", "-1.1", "--v_long", "-s", "42"};
    const auto argv = args.data();
    const int argc = args.size();

    MyArgs argparse(argc, argv);

    ASSERT_TRUE(argparse.parse()) << "Arg parsing failed!";
    EXPECT_DOUBLE_EQ(argparse.long_arg, -1.1);
    EXPECT_EQ(argparse.short_arg, 42);
    EXPECT_EQ(argparse.has_v, true);
    EXPECT_TRUE(argparse.positional_args.empty());
}
