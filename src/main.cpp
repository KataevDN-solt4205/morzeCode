#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <cstring>
#include <errno.h>
#include <sysexits.h>
#include <functional>
#include <mutex>
#include <csignal>
#include "MorzeCoder.hpp"
#include "InterruptibleConsoleReader.hpp"

#define SERVER_PATH "tpf_unix_sock.server"

void usage(const char *app_name)
{
    printf("%s -c / -s\n", app_name);
    printf("\t-c: client\n");
    printf("\t-s: server\n");
}

static bool ClientConsoleOnRead(void *ext_data, std::string &instr)
{
    std::cout << "ClientConsoleOnRead: " << instr << std::endl;
    if ((instr.compare("q") == 0) ||
        (instr.compare("quit") == 0))
    {
        return true;
    }
    return false;
}

int main(int argc, char* argv[])
{
    /* set ANSII locale*/
	setlocale(LC_ALL, "C");

    if (argc != 2){
        usage(argv[0]);
        exit(EX_USAGE);
    }

    MorzeCoder coder;

    /* is client */
    if (!strncmp(argv[1], "-c", 2))
    {
        std::cout << "is client" << std::endl;

        InterruptibleConsoleReader consoleReader(NULL, ConsoleOnRead, SIGUSR1);
  
        consoleReader.StartRead();
        consoleReader.WaitTerminate();
        
        exit(EX_OK);
    }
}