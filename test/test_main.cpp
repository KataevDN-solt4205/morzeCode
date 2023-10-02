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
#include <catch.hpp>

TEST_CASE("Morze code/decode", "[MorzeCoder]")
{
    MorzeCoder coder;
    std::string instr = "abcde fghijklmnop qrstuvwxyz";
    std::string outstr;
    std::vector<uint8_t> encodebuf(128, 0);
    int encode_ret = coder.Encode(instr, encodebuf);
    int decode_ret = coder.Decode(encodebuf, outstr);
    CHECK(encode_ret == 0);
    CHECK(decode_ret == 0);
    CHECK(instr.compare(outstr) == 0);
}

TEST_CASE("Morze decode 2 code", "[MorzeCoder]")
{
    MorzeCoder coder;
    std::string instr = "abcde fghijklmnop qrstuvwxyz";
    std::string outstr;
    std::vector<uint8_t> encodebuf(128, 0);
    int encode_ret = coder.Encode(instr, encodebuf);
    instr += instr;
    encodebuf.insert(encodebuf.end(), encodebuf.begin(), encodebuf.end());
    int decode_ret = coder.Decode(encodebuf, outstr);
    CHECK(encode_ret == 0);
    CHECK(decode_ret == 0);
    CHECK(instr.compare(outstr) == 0);
}


