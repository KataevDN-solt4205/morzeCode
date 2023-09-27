#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"

int main(int argc, char* argv[])
{
    /* set ANSII locale*/
	setlocale(LC_ALL, "C");

    MorzeCoder encoder;

	std::cout <<  "Hello World" << std::endl;

    std::string src = "Hello World";
    std::vector<uint8_t> dst;

    encoder.Encode(src, dst);

	return 0;
}