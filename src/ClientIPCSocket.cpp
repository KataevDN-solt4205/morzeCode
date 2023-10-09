
#include <sys/socket.h>
#include <sys/types.h>
#include <iostream>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <csignal>
#include <vector>
#include <functional>
#include <netdb.h>
#include <mutex>
#include <unistd.h>
#include <string>
#include "BasicReadBufferSupervisor.hpp"
#include "ClientIPCSocket.hpp"

ClientIPCSocket::ClientIPCSocket(void *_ext_data, 
    state_callback_t OnConnect, state_callback_t OnDisconnect, read_callback_t OnRead)       
    :BasicReadBufferSupervisor()
{
    read_buf.clear();
    read_buf.reserve(256);

    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    on_connect    = OnConnect;
    on_disconnect = OnDisconnect;
    on_read       = OnRead;
    ext_data      = _ext_data;
}

ClientIPCSocket::ClientIPCSocket(int fd, void *_ext_data, 
    state_callback_t OnConnect, state_callback_t OnDisconnect, read_callback_t OnRead)       
    :BasicReadBufferSupervisor()
{
    read_buf.clear();
    read_buf.reserve(256);
    Attach(fd);

    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    on_connect    = OnConnect;
    on_disconnect = OnDisconnect;
    on_read       = OnRead;
    ext_data      = _ext_data;
}

ClientIPCSocket::~ClientIPCSocket()
{
    Close();
}

void ClientIPCSocket::SetExtData(void *_ext_data)
{
    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    ext_data      = _ext_data;
}

void ClientIPCSocket::SetOnConnectCallback(state_callback_t OnConnect)
{
    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    on_connect    = OnConnect;
}

void ClientIPCSocket::SetOnDisconnectCallback(state_callback_t OnDisconnect)
{
    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    on_disconnect = OnDisconnect;
}

void ClientIPCSocket::SetOnReadCallback(read_callback_t OnRead)
{
    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    on_read       = OnRead;
}

int ClientIPCSocket::Send(std::vector<uint8_t> &buf)
{
    if (fd == -1){
        return -EBADF;
    }

    int rc = send(fd, buf.data(), buf.size(), 0);
    if (rc == -1) 
    {   
        Close();
        return -errno;
    }  
    return rc;
}

int  ClientIPCSocket::Open()
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

void ClientIPCSocket::Close()
{
    if (IsRunning())
    {
        Stop();
        Join();
    }
    if (fd >= 0){
        close(fd);
    }
    fd = -1;
}

int  ClientIPCSocket::Connect(const std::string uri)
{
    struct sockaddr_un un_addr;
    if (fd == -1){
        return -EBADF;
    }

    size_t max_path = (sizeof(un_addr) - sizeof(un_addr.sun_family));
    if (uri.length() >= max_path){
        return -EINVAL;
    }

    un_addr.sun_family = AF_UNIX;
    strcpy(un_addr.sun_path, uri.c_str());
    size_t len = sizeof(un_addr.sun_family) + uri.length(); 

    if(connect(fd, (struct sockaddr *) &un_addr, len) == -1){
        Close();
        return -errno;
    }
    return 0;
}

int ClientIPCSocket::BeforeThreadLoop()
{
    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    if (on_connect){
        on_connect(ext_data, *this);
    }
    return 0;
}

int ClientIPCSocket::ExistDataInReadBuffer()
{
    int rc;
    do 
    {
        rc = recv(fd, recive_buf, recive_buf_len, 0);
        /* if data exists but rc == 0 then server killed */
        if (rc <= 0){
            return -1;
        } 

        try
        {
            read_buf.insert(read_buf.end(), recive_buf, recive_buf + rc);
        } 
        catch(const std::exception& e)
        {
            return -ENOMEM;
        }
        
        if ((size_t)rc < recive_buf_len) 
        {
            std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
            if (on_read){
                on_read(ext_data, *this, read_buf);
            }
            read_buf.clear();
            break;
        }
    }
    while ((stop_thread == false) && (DataInReadBufExists() >= 0));
    return 0;
}

void ClientIPCSocket::AfterThreadLoop()
{
    std::lock_guard<std::mutex> locker(ext_data_and_callbaks_lock);
    if (on_disconnect){
        on_disconnect(ext_data, *this);
    }
}

