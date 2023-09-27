#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <map>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"

int main(int argc, char* argv[])
{
    /* set ANSII locale*/
	setlocale(LC_ALL, "C");

    MorzeCoder encoder;

	std::cout <<  "Hello World" << std::endl;

    std::string src = "Hellow  12312 World 12312331 фвфывф\t";
    std::vector<uint8_t> dst;
    std::string outstr;

    encoder.Encode(src, dst);
    encoder.Decode(dst, outstr);
    std::cout << outstr << std::endl;
	return 0;
}