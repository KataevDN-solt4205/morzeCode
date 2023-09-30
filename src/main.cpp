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
#include "ClientIPCSocket.hpp"
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
    std::cout << outstr << std::endl;
}

static void ClientSocketOnDisconnect(void *ext_data, ClientIPCSocket &csocket)
{
    ClientContext *ctx = (ClientContext *)ext_data;  
    ctx->consoleReader.StopRead();
    std::cout << "Disconnect." << std::endl;
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

        // std::string in_str;
        // std::vector<uint8_t> encode_buf;
        // try
        // {
        //     MorzeCoder encoder;
        //     IPCSocket socket(SOCKET_ADDRESS);
        //     socket.AsyncWaitConnect(5);
        //     while(1){
        //         std::cin >> in_str;
        //         if (in_str == "exit"){
        //             break;
        //         }
        //         encoder.Encode(in_str, encode_buf);
        //         socket.Send(encode_buf);
        //     }
        // }
        // catch(const std::exception& e)
        // {
        //     std::cerr << e.what() << '\n';
        // }

    int server_sock, client_sock, rc;
    // int bytes_rec = 0;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;     
    char buf[256];
    int backlog = 0;
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buf, 0, 256);                
    
    /**************************************/
    /* Create a UNIX domain stream socket */
    /**************************************/
    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1){
        std::cerr << "SOCKET ERROR: " << gai_strerror(errno) << std::endl;
        exit(1);
    }
    
    /***************************************/
    /* Set up the UNIX sockaddr structure  */
    /* by using AF_UNIX for the family and */
    /* giving it a filepath to bind to.    */
    /*                                     */
    /* Unlink the file so the bind will    */
    /* succeed, then bind to that file.    */
    /***************************************/
    server_sockaddr.sun_family = AF_UNIX;   
    strcpy(server_sockaddr.sun_path, SERVER_PATH); 
    socklen_t len = sizeof(server_sockaddr);
    
    unlink(SERVER_PATH);
    rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
    if (rc == -1){
        std::cerr << "BIND ERROR: " << gai_strerror(errno) << std::endl;
        close(server_sock);
        exit(1);
    }

    /*********************************/
    /* Listen for any client sockets */
    /*********************************/
    rc = listen(server_sock, backlog);
    if (rc == -1){ 
        std::cerr << "LISTEN ERROR: " << gai_strerror(errno) << std::endl;
        close(server_sock);
        exit(1);
    }
    printf("socket listening...\n");

   
    /*********************************/
    /* Accept an incoming connection */
    /*********************************/

    printf("before accept  !!!!!!!!!!!!!!!!!!\n");
    client_sock = accept(server_sock, (struct sockaddr *) &client_sockaddr, &len);
    if (client_sock == -1){
        std::cerr << "ACCEPT ERROR: " << gai_strerror(errno) << std::endl;
        close(server_sock);
        close(client_sock);
        exit(1);
    }
    printf("after accept  !!!!!!!!!!!!!!!!!!\n");
    
    /******************************************/
    /* Send data back to the connected socket */
    /******************************************/
	std::cout <<  "Hello World" << std::endl;

    std::string src = "Hellow  12312 World 12312331 фвфывф\t";
    std::vector<uint8_t> dst;
    while(1){
        sleep(1);
        memset(buf, 0, 256);
        strcpy(buf, "DATA");      
        printf("Sending data...\n");
        coder.Encode(src, dst);
        rc = send(client_sock, dst.data(), dst.size(), 0);
        if (rc == -1) {
            std::cerr << "SEND ERROR: " << gai_strerror(errno) << std::endl;
            close(server_sock);
            close(client_sock);
            exit(1);
        }   
        else {
            printf("Data sent!\n");
        }

    };
    
    /******************************/
    /* Close the sockets and exit */
    /******************************/
    close(server_sock);
    close(client_sock);
    exit(EX_OK);
    }

    usage(argv[0]);
    exit(EX_USAGE);


}
