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

void TestEncode(IEncoder &encoder, std::string in_str, std::string string_of_bits) 
{
    std::vector<uint8_t> encodebuf(128, 0);
    int encode_ret = encoder.Encode(in_str, encodebuf);  

    std::string encode_str;
    encoder.EncodeBufToStr(encodebuf, encode_str);

    INFO( in_str )
    INFO( encode_str )
    INFO( string_of_bits )

    CHECK(encode_ret == 0);
    CHECK(encode_str.compare(string_of_bits) == 0);
}

TEST_CASE("Morze encode", "[MorzeEncode]")
{
    MorzeCoder coder;
    IEncoder &encoder = (IEncoder&)coder;

    /*                                  0       1       2       3       4       5       6       7       8       9       10*/
    /*                                  0       8       16      24      32      40      48      56      64      72      80*/
    TestEncode(encoder, "qwerty",      "111011101011100010111011100010001011101000111000111010111011100000000000");
    TestEncode(encoder, "q werty",     "111011101011100000001011101110001000101110100011100011101011101110000000");
    TestEncode(encoder, "q  werty",    "111011101011100000001011101110001000101110100011100011101011101110000000");
    TestEncode(encoder, "q werty ",    "111011101011100000001011101110001000101110100011100011101011101110000000");
    TestEncode(encoder, "q wрусerty",  "111011101011100000001011101110001000101110100011100011101011101110000000");
    TestEncode(encoder, "q w123erty",  "111011101011100000001011101110001000101110100011100011101011101110000000");
    TestEncode(encoder, "q w123e  rty","11101110101110000000101110111000100000001011101000111000111010111011100000000000");
    TestEncode(encoder, "q w123erty  ","111011101011100000001011101110001000101110100011100011101011101110000000");
}

void TestDecode(IEncoder &encoder, IDecoder &decoder, std::string in_str, std::string out_str)
{
    std::vector<uint8_t> encodebuf(128, 0);
    int encode_ret = encoder.Encode(in_str, encodebuf); 
    
    std::string tmp_out;
    int decode_ret = decoder.Decode(encodebuf, tmp_out); 

    std::string encode_str;
    encoder.EncodeBufToStr(encodebuf, encode_str);

    INFO( in_str )
    INFO( tmp_out )
    INFO( out_str )
    CHECK(encode_ret == decode_ret);
    CHECK(out_str.compare(tmp_out) == 0);
}

void TestDecode2Msg(IEncoder &encoder, IDecoder &decoder, std::string in_str, std::string out_str)
{
    std::vector<uint8_t> encodebuf(128, 0);
    int encode_ret = encoder.Encode(in_str, encodebuf); 
    encodebuf.insert(encodebuf.end(), encodebuf.begin(), encodebuf.end());
    
    std::string tmp_out;
    int decode_ret = decoder.Decode(encodebuf, tmp_out); 

    std::string encode_str;
    encoder.EncodeBufToStr(encodebuf, encode_str);

    INFO( in_str )
    INFO( tmp_out )
    INFO( out_str )
    CHECK(encode_ret == decode_ret);
    CHECK(out_str.compare(tmp_out) == 0);
}

TEST_CASE("Morze decode", "[MorzeDecoder]")
{
    MorzeCoder coder;
    IEncoder &encoder = (IEncoder&)coder;
    IDecoder &decoder = (IDecoder&)coder;

    TestDecode(encoder, decoder, "qwerty", "qwerty ");
    TestDecode(encoder, decoder, "q werty", "q werty ");
    TestDecode(encoder, decoder, "q  werty", "q werty ");
    TestDecode(encoder, decoder, "q we rty ", "q we rty ");
    TestDecode(encoder, decoder, "qwerty  ", "qwerty ");
    TestDecode(encoder, decoder, "qwe123 rty", "qwe rty ");
    TestDecode(encoder, decoder, "qwe123rty", "qwerty ");
    TestDecode(encoder, decoder, "qwрусerty", "qwerty ");

    TestDecode2Msg(encoder, decoder, "qwerty", "qwerty qwerty ");
    TestDecode2Msg(encoder, decoder, "q werty  ", "q werty q werty ");
    TestDecode2Msg(encoder, decoder, "q  werty  ", "q werty q werty ");

    std::vector<uint8_t> original_encodebuf(128, 0), encodebuf;
    std::string in_str = "q w ", out_str = "", tmp_str = "";
    int decode_ret = 0;
    
    encoder.Encode(in_str, original_encodebuf);  
    /*           0       1       2       3       4       5 */
    /*           0       8       16      24      32      40*/
    /* code str: 1110111010111000000010111011100000000000  */

    /* error on letter delim */
    encodebuf = original_encodebuf;
    encodebuf[1] |= 0x40;
    decode_ret = decoder.Decode(encodebuf, out_str);
    encoder.EncodeBufToStr(encodebuf, tmp_str);
    INFO(in_str);
    INFO(tmp_str);
    INFO(out_str);
    CHECK_FALSE(decode_ret == 0);

    /* error on word delim */
    encodebuf = original_encodebuf;
    encodebuf[2] |= 0x02;
    decode_ret = decoder.Decode(encodebuf, out_str);
    encoder.EncodeBufToStr(encodebuf, tmp_str);
    INFO(in_str);
    INFO(tmp_str);
    INFO(out_str);
    CHECK_FALSE(decode_ret == 0);

    /* error on unk. letter */
    encodebuf = original_encodebuf;
    encodebuf[0] &= 0xDD;
    decode_ret = decoder.Decode(encodebuf, out_str);
    encoder.EncodeBufToStr(encodebuf, tmp_str);
    INFO(in_str);
    INFO(tmp_str);
    INFO(out_str);
    CHECK_FALSE(decode_ret == 0);
}

