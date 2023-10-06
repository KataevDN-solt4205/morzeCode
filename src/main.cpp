#include <iostream>
#include <fstream>
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
#include <termios.h>
#include "ICoder.hpp"
#include "MorzeCoder.hpp"
#include "ClientIPCSocket.hpp"
#include "ServerIPCSocket.hpp"
#include "InterruptibleConsoleReader.hpp"

#define SERVER_SOCKET_PATH "/tmp/tpf_unix_sock.server"
#define SERVER_PID_PATH    "/tmp/tpf_unix_sock.pid"
#define MAX_CLIENTS        100

void usage(const char *app_name)
{
    printf("%s -c / -s max_clients\n", app_name);
    printf("\t-c: client\n");
    printf("\t-s: server\n");
    printf("\tmax_clients: maximum clients that the server can serve (no more than %d)\n", MAX_CLIENTS);
}

/*---------------------------CLIENT-----------------------------*/
typedef struct 
{
    InterruptibleConsoleReader &consoleReader;
    IDecoder &decoder;
} ClientContext;

static void ClientSocketOnRead(void *ext_data, ClientIPCSocket &csocket, std::vector<uint8_t> &buf)
{
    ClientContext *ctx = (ClientContext *)ext_data;
    std::string outstr;
    if (buf.size() > 0)
    {
        int rc = ctx->decoder.Decode(buf, outstr);
        if (rc){
            return;
        }
        std::cout << "Recive: " << outstr << std::endl;
    }
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

int client()
{
    int rc = 0;    
    MorzeCoder coder;
    IDecoder &decoder = (IDecoder&)coder;     
    ClientIPCSocket client_socket; 
    InterruptibleConsoleReader consoleReader(NULL, ClientConsoleOnRead, SIGUSR1);
    ClientContext ctx = {.consoleReader=consoleReader, .decoder=decoder};

    rc = client_socket.Open();
    if (rc < 0){
        std::cerr << "OPEN ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }
    rc = client_socket.Connect(SERVER_SOCKET_PATH);
    if (rc < 0){
        std::cerr << "CONNECT ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }

    client_socket.SetExtData(&ctx);
    client_socket.SetCallbacs(NULL, ClientSocketOnDisconnect, ClientSocketOnRead);

    rc = client_socket.StartReading();
    if (rc < 0){
        std::cerr << "START READING ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }
       
    std::cout << "client: for quit send 'q' or 'quit'." << std::endl;

    consoleReader.StartRead();
    consoleReader.WaitTerminate();
    
    client_socket.Close();
    return 0;
}

/*---------------------------SERVER-----------------------------*/

typedef struct 
{
    IEncoder             &encoder;
    ServerIPCSocket      &server;
    std::vector<uint8_t> &outbuf;
    std::string          &use_string;
    std::string          &morze_data;
} ServerContext;

static void ServerOnClientConnect(void *ext_data, int client_count, ClientIPCSocket *client)
{
    printf("Connect new client: %p, client count %d\n", client, client_count);
}

static void ServerOnClientDisconnect(void *ext_data, int client_count, ClientIPCSocket *client)
{
    printf("Disconnect client: %p, client count %d\n", client, client_count);
}

static bool ServerConsoleOnRead(void *ext_data, std::string &instr)
{
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
        if (((c >= 'a') && (c <= 'z')) || (c == ' ')){
            ctx->use_string += c;
            continue;
        }
        if ((c >= 'A') && (c <= 'Z')){
            ctx->use_string += c - 'A' + 'a';
            continue;
        }
    }

    if (ctx->use_string.length()){
        std::cout << "Encode string: " << ctx->use_string << std::endl;
        ctx->encoder.Encode(instr, ctx->outbuf);
        ctx->encoder.EncodeBufToStr(ctx->outbuf, ctx->morze_data);
        std::cout << "Morze data: " << ctx->morze_data << std::endl;
        if (ctx->outbuf.size())
        {
            ctx->server.SendAll(ctx->outbuf);
        }
    }

    return false;
}

bool server_alredy_running(const std::string pid_file_path)
{
    pid_t pid;
    std::ifstream input(pid_file_path);

    if (input.is_open()){
        input >> pid;
        /* pid in file exists*/
        if (pid > 0){
            /* app terminate*/
            if ((kill(pid, 0) == -1) && (errno == ESRCH)){          
                return false; 
            }
        }        
        return true;
    }
    return false;
}

void write_my_pid(const std::string pid_file_path, const pid_t my_pid)
{
    std::ofstream output(pid_file_path);
    if (output.is_open()){
        output << my_pid;
    }
}

int server(int max_clients)
{
    if (max_clients > MAX_CLIENTS){
        std::cout << "the maximum number of clients cannot be more than " << MAX_CLIENTS << std::endl;
        return -EINVAL;
    }

    if (server_alredy_running(SERVER_PID_PATH)){
        std::cout << "server alredy started." << std::endl;
        return -EEXIST;
    }
    write_my_pid(SERVER_PID_PATH, getpid());

    int rc = 0;
    MorzeCoder coder;
    IEncoder &encoder = (IEncoder&)coder;
    ServerIPCSocket server;  
    std::vector<uint8_t> outbuf(1024, 0);
    std::string          use_string(1024, 0);
    std::string          morze_data(1024, 0);
    ServerContext ctx = 
    {
        .encoder=encoder, 
        .server=server, 
        .outbuf=outbuf, 
        .use_string=use_string,
        .morze_data=morze_data
    };     
    InterruptibleConsoleReader consoleReader((void*)&ctx, ServerConsoleOnRead, SIGUSR1);

    rc = server.Open();
    if (rc < 0){
        std::cerr << "OPEN ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }

    rc = server.Bind(SERVER_SOCKET_PATH);
    if (rc < 0){
        std::cerr << "BIND ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }

    rc = server.Listen();
    if (rc < 0){
        std::cerr << "LISTEN ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }

    server.SetCallback(ServerOnClientConnect, ServerOnClientDisconnect);

    rc = server.StartAcceptThread(max_clients);
    if (rc < 0){
        std::cerr << "ACCEPT ERROR: " << gai_strerror(rc) << rc << std::endl;
        return rc;
    }

    std::cout << "server: for quit send 'q' or 'quit'." << std::endl;

    consoleReader.StartRead();
    consoleReader.WaitTerminate();        
    
    server.Close();
    return 0;

}

int main(int argc, char* argv[])
{
    /* set ANSII locale*/
	setlocale(LC_ALL, "C");

    if (argc < 2){
        usage(argv[0]);
        exit(EX_USAGE);
    }

    /* is client */
    if (!strncmp(argv[1], "-c", 2))
    {
        if (client()){
            exit(EX__BASE);
        }
        exit(EX_OK);
    } 

    /* is server*/
    if (!strncmp(argv[1], "-s", 2))
    {
        if (argc != 3){
            usage(argv[0]);
            exit(EX_USAGE);
        }

        uint32_t max_clients = strtoul(argv[2], NULL, 0);
        if (max_clients > MAX_CLIENTS){
            usage(argv[0]);
            exit(EX_USAGE);
        }
        
        if (server(max_clients)){
            exit(EX__BASE);
        }
        exit(EX_OK);
    }
 
    usage(argv[0]);
    exit(EX_USAGE);
}