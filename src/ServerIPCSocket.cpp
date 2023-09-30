#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <csignal>
#include <vector>
#include <functional>
#include <algorithm>
#include <netdb.h>
#include <mutex>
#include <unistd.h>
#include <string>
#include "ClientIPCSocket.hpp"
#include "ServerIPCSocket.hpp"

ServerIPCSocket::ServerIPCSocket()
    : socket_fd(-1),
    thread_interrupt_signal(-1),
    accept_thread(-1)
{
    clients.clear();
}

ServerIPCSocket::~ServerIPCSocket()
{
    SocketClose();
}

int ServerIPCSocket::Open()
{
    if (socket_fd != -1){
        return -EEXIST;
    }

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        return -errno;
    }
    return 0;
}

void ServerIPCSocket::SocketClose()
{
    std::lock_guard<std::mutex> locker(_lock);
    /* close and delete all clients */
    for (ClientIPCSocket *client: clients){
        delete(client);
    }
    clients.clear();

    /* close server fd */
    if (socket_fd >= 0)
    {
        close(socket_fd);
    }
    socket_fd = -1;
}

int ServerIPCSocket::Bind(const std::string sock_path)
{
    un_addr.sun_family = AF_UNIX;   
    strcpy(un_addr.sun_path, sock_path.c_str()); 
    socklen_t len = sizeof(un_addr);
    
    unlink(sock_path.c_str());
    int rc = bind(socket_fd, (struct sockaddr *) &un_addr, len);
    if (rc == -1){
        SocketClose();
        return -errno;
    }
    return 0;
}

int ServerIPCSocket::Listen(const int backlog)
{
    int rc = listen(socket_fd, backlog);
    if (rc == -1){
        SocketClose();
        return -errno;
    }
    return 0;
    printf("socket listening...\n");
}

int  ServerIPCSocket::WaitDataForAcceptInfinity(int socket_fd, sigset_t sig_mask)
{
    fd_set readfds;
       
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    int rc = pselect(socket_fd+1, &readfds, NULL, NULL, NULL, &sig_mask);
    return rc;
}

int ServerIPCSocket::Accept(int &client_fd, struct sockaddr_un &client_sockaddr, sigset_t &sig_mask)
{
    socklen_t len = sizeof(client_sockaddr);

    /* wait infinity*/
    int data_exists = WaitDataForAcceptInfinity(socket_fd, sig_mask);
    if (data_exists >= 0)
    {        
        client_fd = accept(socket_fd, (struct sockaddr *) &client_sockaddr, &len);
        if (client_fd == -1){
            return -errno;
        }
    }
    return 0;
}

void ServerIPCSocket::OnClientConnect(void *ext_data, ClientIPCSocket &client)
{
    // ServerIPCSocket *server = (ServerIPCSocket *)ext_data;
    // std::lock_guard<std::mutex> locker(_lock);
}

void ServerIPCSocket::OnClientDisconnect(void *ext_data, ClientIPCSocket &client)
{
    ServerIPCSocket *server = (ServerIPCSocket *)ext_data;
    std::lock_guard<std::mutex> locker(server->_lock);

    std::vector<ClientIPCSocket*>::iterator pos = std::find(server->clients.begin(), server->clients.end(), &client);
    if (pos != server->clients.end()){
        delete(*pos);
        server->clients.erase(pos);
    }
}
         
void ServerIPCSocket::OnClientRead(void *ext_data, ClientIPCSocket &client, std::vector<uint8_t> &buf)
{
    // ServerIPCSocket *server = (ServerIPCSocket *)ext_data;
    // std::lock_guard<std::mutex> locker(_lock);
}

static void ServerIPCSocketSIGUSR(int signo) {(void)signo;}
void *ServerIPCSocket::AcceptThreadFunction()
{
    int rc = 0;
    int client_fd;
    struct sockaddr_un client_sockaddr;

    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigemptyset(&oldset);    
    sigaddset(&sigset, thread_interrupt_signal );
    signal(thread_interrupt_signal, ServerIPCSocketSIGUSR);
    sigprocmask(SIG_BLOCK, &sigset, &oldset);  

    while (!stop_accept)
    {        
        rc = Accept(client_fd, client_sockaddr, oldset);
        if (rc <= 0){
            break;
        }
        {
            std::lock_guard<std::mutex> locker(_lock);
            /* mo then max drop it */
            if (clients.size() >= max_clients){
                close(client_fd);
            }
            
            /* create client */
            ClientIPCSocket *new_client = new ClientIPCSocket(client_fd, client_sockaddr);
            /* configure */
            new_client->SetExtData(this);
            new_client->SetCallbacs(OnClientConnect, OnClientDisconnect, OnClientRead);
            new_client->StartReading();
            /* add */
            clients.push_back(new_client);
        }
    }
    SocketClose();
    return NULL;
}

void *ServerIPCSocket::StaticAcceptThreadFunction(void *_ctx)
{
    ServerIPCSocket *ctx = (ServerIPCSocket *)_ctx;  
    pthread_exit(ctx->AcceptThreadFunction());
    ctx->accept_thread = (pthread_t)-1;
}

int ServerIPCSocket::StartAcceptThread(const int _max_clients)
{
    if (socket_fd == -1){
        return -EBADF;
    }

    if (accept_thread != (pthread_t)-1){
        return -EEXIST;
    }
    max_clients = _max_clients;
    stop_accept = false;
    return pthread_create(&accept_thread, NULL, &StaticAcceptThreadFunction, this);
}

int ServerIPCSocket::SendAll(std::vector<uint8_t> &buf)
{
    std::lock_guard<std::mutex> locker(_lock);

    for(ClientIPCSocket *client: clients)
    {
        printf("Send %p %zu\n", client, buf.size());
        client->Send(buf);
    }

    return 0;
}

void ServerIPCSocket::Close()
{
    /* only stop thread disconnect in thread */
    stop_accept = true;

    if (accept_thread != (pthread_t)-1){
        pthread_kill(accept_thread, SIGUSR1);
        pthread_join(accept_thread, NULL); 
        accept_thread = -1;
    }
}

void ServerIPCSocket::SetExtData(void *_ext_data)
{
    ext_data = _ext_data;
}