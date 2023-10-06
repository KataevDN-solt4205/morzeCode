#define CATCH_CONFIG_MAIN
#include <vector>
#include <stdint.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <map>
#include <signal.h>
#include <functional>
#include <sys/types.h>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"
#include "catch.hpp"

TEST_CASE("Morze code/decode", "[MorzeCoder]")
{
    MorzeCoder coder;
    std::string instr = "abcdefghijklmnopqrstuvwxyz";
    std::string outstr;
    std::vector<uint8_t> encodebuf(128, 0);

    int encode_ret = coder.Encode(instr, encodebuf);
    int decode_ret = coder.Decode(encodebuf, outstr);

    CHECK(encode_ret == 0);
    CHECK(decode_ret == 0);
    CHECK(instr.compare(outstr) == 0);
}

void morze(MorzeCoder &coder, std::string in, std::string &out)
{
    out.clear();
    std::vector<uint8_t> encodebuf(128, 0);
    coder.Encode(in, encodebuf);
    coder.Decode(encodebuf, out);
}

TEST_CASE("Morze decode", "[MorzeDecode]")
{
    MorzeCoder coder;
    std::string outstr;

    morze(coder, "123123  asdfg 234 ", outstr);
    std::cout << "[" << outstr << "]" <<std::endl;
    CHECK(0 == outstr.compare("asdfg "));

    morze(coder, "dasdasdaывфывф123123     asdfg 234 ", outstr);
    std::cout << "[" << outstr << "]" <<std::endl;
    CHECK(0 == outstr.compare("dasdasd asdfg "));
}

