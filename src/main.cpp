#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <cstring>
#include <sysexits.h>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"
#include <getopt.h>

#define DEFAULT_PORT 4902

typedef struct 
{
    int port;
    bool is_server;
    bool is_client;
} params_t;

void usage(const char *app_name)
{
    printf("%s [-p port] -c / -s\n", app_name);
    printf("\t-p: socket port\n");
    printf("\t-c: client\n");
    printf("\t-s: server\n");
}

void parse_params(int argc, char *argv[], params_t &params)
{
    int ret;
    memset(&params, 0, sizeof(params_t));

    while ((ret =getopt(argc, argv, "p:sc")) != -1)
    {
        switch(ret)
        {
            case 'p':
                params.port = (optarg ? strtol(optarg, NULL, 0) : DEFAULT_PORT);
                break;
            case 's':
                params.is_server = true;
                break;
            case 'c':
                params.is_client = true;
                break;
            default:
                throw std::invalid_argument("Unk. args");
        }
    }

    if (params.is_server == params.is_client){
        throw std::invalid_argument("Unk. args");
    }
}


int main(int argc, char* argv[])
{
    /* set ANSII locale*/
	setlocale(LC_ALL, "C");

    params_t params;
    try{
        parse_params(argc, argv, params);
    }
    catch (const std::invalid_argument& error)
    {
        usage(argv[0]);
        exit(EX_USAGE);
    } 

    MorzeCoder encoder;

	std::cout <<  "Hello World" << std::endl;

    std::string src = "Hellow  12312 World 12312331 фвфывф\t";
    std::vector<uint8_t> dst;
    std::string outstr;

    encoder.Encode(src, dst);
    encoder.Decode(dst, outstr);
    std::cout << outstr << std::endl;
	exit(EX_OK);
}