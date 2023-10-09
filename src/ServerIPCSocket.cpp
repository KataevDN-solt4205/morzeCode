#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <iostream>
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
#include "BasicReadBufferSupervisor.hpp"
#include "ClientIPCSocket.hpp"
#include "ServerIPCSocket.hpp"

ServerIPCSocket::ServerIPCSocket(void *_ext_data, state_callback_t OnConnect, state_callback_t OnDisconnect)
    :BasicReadBufferSupervisor(),
    ext_data(_ext_data),
    on_connect(OnConnect),
    on_disconnect(OnDisconnect)
{
    clients.clear();
    deleted_clients.clear();
}

ServerIPCSocket::~ServerIPCSocket()
{    
    Close();
}

int ServerIPCSocket::Open()
{
    if (fd != -1){
        return -EEXIST;
    }

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        return -errno;
    }
    return 0;
}

void ServerIPCSocket::Close()
{  
    /* terminate server thread*/
    if (IsRunning()){
        Stop();
        Join();
    }

    /* stop all clients */
    for (ClientIPCSocket *client: clients){
        /* disable move client to deleted list*/
        client->SetOnDisconnectCallback(NULL);
        client->Stop();
    }

    /* delete all clients */
    for (ClientIPCSocket *client: clients){
        client->Join();
        delete(client);
    }
    clients.clear();

    /* delete all deleted clients */
    FreeDeletedClient();

    /* close server fd */
    if (fd >= 0){
        close(fd);
    }
    fd = -1;
}

int ServerIPCSocket::Bind(const std::string uri)
{
    struct sockaddr_un un_addr;
    un_addr.sun_family = AF_UNIX;   
    strcpy(un_addr.sun_path, uri.c_str()); 
    socklen_t len = sizeof(un_addr);
    
    unlink(uri.c_str());
    int rc = bind(fd, (struct sockaddr *) &un_addr, len);
    if (rc == -1){
        Close();
        return -errno;
    }
    return 0;
}

int ServerIPCSocket::Listen(size_t _max_clients)
{
    max_clients = _max_clients;
    int rc = listen(fd, backlog);
    if (rc == -1){
        Close();
        return -errno;
    }
    return 0;
}

int ServerIPCSocket::Accept(int &client_fd)
{
    struct sockaddr client_sockaddr;
    socklen_t len = sizeof(client_sockaddr);
    client_fd = accept(fd, (struct sockaddr *) &client_sockaddr, &len);
    if (client_fd == -1){
        Close();
        return -errno;
    }
    return 0;
}

void ServerIPCSocket::FreeDeletedClient()
{
    std::lock_guard<std::mutex> locker(list_lock);
    for (ClientIPCSocket *c: deleted_clients){
        delete(c);
    }
    deleted_clients.clear();
}

void ServerIPCSocket::OnClientDisconnect(void *ext_data, ClientIPCSocket &client)
{
    ServerIPCSocket *server = (ServerIPCSocket *)ext_data;
    ClientIPCSocket *client_ptr = &client;
    
    std::lock_guard<std::mutex> locker(server->list_lock);
    std::vector<ClientIPCSocket*>::iterator pos = std::find(server->clients.begin(), server->clients.end(), client_ptr);
    if (pos != server->clients.end()){
        server->clients.erase(pos);
        
        if (server->on_disconnect){
            server->on_disconnect(server->ext_data, server->clients.size(), client_ptr);
        }
        /* удалить самого себя мы не можем */
        /* сохраним указатель в список удаления */ 
        server->deleted_clients.push_back(client_ptr);
    }
}

int ServerIPCSocket::ExistDataInReadBuffer()
{
    FreeDeletedClient();
    int client_fd = -1;
    int rc = Accept(client_fd);
    if (rc == 0)
    {
        /* mo then max drop it */
        if (clients.size() >= max_clients){
            close(client_fd);
            return rc;
        }
        
        /* create client */
        ClientIPCSocket *new_client = new ClientIPCSocket(client_fd, this, NULL, OnClientDisconnect, NULL);
        new_client->Start();
              
        if (on_connect){
            on_connect(ext_data, clients.size()+1, new_client);
        }

        client_fd = -1;
        std::lock_guard<std::mutex> locker(list_lock);
        clients.push_back(new_client);        
    } 
    return rc;
}

int ServerIPCSocket::Send(std::vector<uint8_t> &buf)
{
    std::lock_guard<std::mutex> locker(list_lock);
    for(ClientIPCSocket *client: clients){
        client->Send(buf);
    }

    return 0;
}
