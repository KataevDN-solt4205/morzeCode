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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"
#include "ClientIPCSocket.hpp"
#include "ServerIPCSocket.hpp"
#include "InterruptibleConsoleReader.hpp"

#define SERVER_PATH "tpf_unix_sock.server"

void usage(const char *app_name)
{
    printf("%s -c / -s\n", app_name);
    printf("\t-c: client\n");
    printf("\t-s: server\n");
}

typedef struct 
{
    InterruptibleConsoleReader &consoleReader;
    MorzeCoder &coder;
} ClientContext;

static void ClientSocketOnRead(void *ext_data, ClientIPCSocket &csocket, std::vector<uint8_t> &buf)
{
    ClientContext *ctx = (ClientContext *)ext_data;
    std::string outstr;
    int rc = ctx->coder.Decode(buf, outstr);
    if (rc){
        return;
    }
    std::cout << "Recive: " << outstr << std::endl;
}

static void ClientSocketOnDisconnect(void *ext_data, ClientIPCSocket &csocket)
{
    ClientContext *ctx = (ClientContext *)ext_data;  
    ctx->consoleReader.StopRead();
    std::cout << "Disconnect." << std::endl;
}

static bool ClientConsoleOnRead(void *ext_data, std::string &instr)
{
    std::cout << "read comand: " << instr << std::endl;
    if ((instr.compare("q") == 0) ||
        (instr.compare("quit") == 0))
    {
        return true;
    }
    return false;
}

typedef struct 
{
    MorzeCoder           &coder;
    ServerIPCSocket      &server;
    std::vector<uint8_t> &outbuf;
    std::string          &use_string;
} ServerContext;

static bool ServerConsoleOnRead(void *ext_data, std::string &instr)
{
    std::cout << "ServerConsoleOnRead: " << instr << std::endl;

    if ((instr.compare("q") == 0) ||
        (instr.compare("quit") == 0))
    {
        std::cout << "quit" << std::endl;
        return true;
    }

    ServerContext *ctx = (ServerContext *)ext_data;

    ctx->use_string.clear();
    ctx->outbuf.clear();

    /* may use only small english ascii */
    for(char c: instr){
        if ((c >= 'a') && (c <= 'z')){
            ctx->use_string += c;
        }
        else if ((c >= 'A') && (c <= 'Z')){
            ctx->use_string += c - 'A' + 'a';
        }
    }
    if (ctx->use_string.length()){
        std::cout << "Encode: " << ctx->use_string << std::endl;
        ctx->coder.Encode(instr, ctx->outbuf);
        if (ctx->outbuf.size())
        {
            ctx->server.SendAll(ctx->outbuf);
        }
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

        int rc = 0;        
        ClientIPCSocket client_socket; 
        InterruptibleConsoleReader consoleReader(NULL, ClientConsoleOnRead, SIGUSR1);
        ClientContext ctx = {.consoleReader=consoleReader, .coder=coder};

        rc = client_socket.Open();
        if (rc < 0){
            std::cerr << "OPEN ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX_CANTCREAT);
        }
        rc = client_socket.Connect(SERVER_PATH);
        if (rc < 0){
            std::cerr << "CONNECT ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX__BASE);
        }

        client_socket.SetExtData(&ctx);
        client_socket.SetCallbacs(NULL, ClientSocketOnDisconnect, ClientSocketOnRead);

        rc = client_socket.StartReading();
        if (rc < 0){
            std::cerr << "START READING ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX__BASE);
        }

        consoleReader.StartRead();
        consoleReader.WaitTerminate();
        
        client_socket.Close();
        exit(EX_OK);
    } 

 /* is server*/
    if (!strncmp(argv[1], "-s", 2))
    {
        std::cout << "is server" << std::endl;

        int rc = 0;
        int max_connections = 5;
        ServerIPCSocket server;  
        std::vector<uint8_t> outbuf(1024, 0);
        std::string          use_string(1024, 0);
        ServerContext ctx = {.coder=coder, .server=server, .outbuf=outbuf, .use_string=use_string};     
        InterruptibleConsoleReader consoleReader((void*)&ctx, ServerConsoleOnRead, SIGUSR1);

        rc = server.Open();
        if (rc < 0){
            std::cerr << "OPEN ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX_CANTCREAT);
        }

        rc = server.Bind(SERVER_PATH);
        if (rc < 0){
            std::cerr << "BIND ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX__BASE);
        }

        rc = server.Listen(max_connections);
        if (rc < 0){
            std::cerr << "LISTEN ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX__BASE);
        }

        rc = server.StartAcceptThread(max_connections);
        if (rc < 0){
            std::cerr << "ACCEPT ERROR: " << gai_strerror(rc) << rc << std::endl;
            exit(EX__BASE);
        }

        consoleReader.StartRead();
        consoleReader.WaitTerminate();
        
        server.Close();
        exit(EX_OK);
    }
 
    usage(argv[0]);
    exit(EX_USAGE);
}