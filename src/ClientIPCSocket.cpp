
#include <sys/socket.h>
#include <sys/types.h>
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
#include "ClientIPCSocket.hpp"

ClientIPCSocket::ClientIPCSocket()
    : socket_fd(-1),
    thread_interrupt_signal(SIGUSR1),
    read_thread(-1)
{
}

ClientIPCSocket::ClientIPCSocket(int fd, struct sockaddr_un &addr)
    : socket_fd(fd),
    un_addr(addr),
    thread_interrupt_signal(SIGUSR1),
    read_thread(-1)
{
}

ClientIPCSocket::~ClientIPCSocket()
{
    Close();
}

int  ClientIPCSocket::Open()
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

void ClientIPCSocket::SocketClose()
{
    if (socket_fd >= 0)
    {
        close(socket_fd);

        std::lock_guard<std::mutex> locker(_lock);
        if (on_disconnect)
        {
            on_disconnect(ext_data, *this);
        }
    }
    socket_fd = -1;
}

int  ClientIPCSocket::WaitDataForReciveInfinity(int socket_fd, sigset_t sig_mask)
{
    fd_set readfds;
       
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    int rc = pselect(socket_fd+1, &readfds, NULL, NULL, NULL, &sig_mask);
    return rc;
}

int  ClientIPCSocket::DataForReciveExists(int socket_fd)
{
    fd_set readfds;
    struct timeval tv = {.tv_sec=0, .tv_usec=0};
        
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    return select(socket_fd+1, &readfds, NULL, NULL, &tv);
}

/* exit on SocketClose | interrupt | error */
int ClientIPCSocket::Recive(std::vector<uint8_t> &buf, sigset_t sig_mask)
{
    buf.clear();
    int rc = 0;
    /* wait infinity*/
    int data_exists = WaitDataForReciveInfinity(socket_fd, sig_mask);
    while (data_exists >= 0)
    {
        rc = recv(socket_fd, recive_buf, recive_buf_len, 0);
        if (rc == -1) 
        {
            SocketClose();
            return -errno;
        } 
        /* if data exists but rc == 0 then server killed */
        if (rc == 0) 
        {
            SocketClose();
            return buf.size();
        }

        try
        {
            buf.insert(buf.end(), recive_buf, recive_buf + rc);
        } 
        catch(const std::exception& e)
        {
            return -ENOMEM;
        }
        
        /* is may be not all message */
        if (rc == recive_buf_len) {
            data_exists = DataForReciveExists(socket_fd);
            continue;
        }

        /* all message recived */
        break;
    }
    /* interrupt exit */
    if (data_exists < 0){
        return -errno;
    }
    return (int)buf.size();
}

void ClientIPCSocket::ClientIPCSocketSignalStopper(int signo) 
{
    (void)signo;
}

void *ClientIPCSocket::ReadThreadFunction()
{
    int rc = 0;
    std::vector<uint8_t> buf;

    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigemptyset(&oldset);    
    sigaddset(&sigset, thread_interrupt_signal );
    signal(thread_interrupt_signal, ClientIPCSocketSignalStopper);
    sigprocmask(SIG_BLOCK, &sigset, &oldset);  

    {
        std::lock_guard<std::mutex> locker(_lock);
        if (on_connect){
            on_connect(ext_data, *this);
        }
    }
    
    while (!stop_read)
    {        
        buf.clear();
        rc = Recive(buf, oldset);
        if (rc < 0){
            break;
        }
        {
            std::lock_guard<std::mutex> locker(_lock);
            if (on_read){
                on_read(ext_data, *this, buf);
            }
        }
    }
    SocketClose();
    return NULL;
}

void *ClientIPCSocket::StaticReadThreadFunction(void *_ctx)
{
    ClientIPCSocket *ctx = (ClientIPCSocket *)_ctx;  
    pthread_exit(ctx->ReadThreadFunction());
    ctx->read_thread = (pthread_t)-1;
}

int ClientIPCSocket::Send(std::vector<uint8_t> &buf)
{
    if (socket_fd == -1){
        return -EBADF;
    }

    int rc = send(socket_fd, buf.data(), buf.size(), 0);
    if (rc == -1) 
    {
        SocketClose();
        return -errno;
    }  
    return rc;
}

int  ClientIPCSocket::Connect(const std::string sock_path)
{
    if (socket_fd == -1){
        return -EBADF;
    }

    size_t max_path = (sizeof(un_addr) - sizeof(un_addr.sun_family));
    if (sock_path.length() >= max_path){
        return -EINVAL;
    }

    un_addr.sun_family = AF_UNIX;
    strcpy(un_addr.sun_path, sock_path.c_str());
    size_t len = sizeof(un_addr.sun_family) + sock_path.length(); 

    if(connect(socket_fd, (struct sockaddr *) &un_addr, len) == -1){
        SocketClose();
        return -errno;
    }
    return 0;
}

int  ClientIPCSocket::StartReading()
{
    if (socket_fd == -1){
        return -EBADF;
    }

    if (read_thread != (pthread_t)-1){
        return -EEXIST;
    }

    stop_read = false;
    return pthread_create(&read_thread, NULL, &StaticReadThreadFunction, this);
}

void ClientIPCSocket::Close()
{
    /* only stop thread disconnect in thread */
    stop_read = true;

    if (read_thread != (pthread_t)-1){
        pthread_kill(read_thread, SIGUSR1);
        pthread_join(read_thread, NULL); 
        read_thread = -1;
    }
}

void ClientIPCSocket::SetExtData(void *_ext_data)
{
    std::lock_guard<std::mutex> locker(_lock);

    ext_data = _ext_data;
}

void ClientIPCSocket::SetCallbacs(state_callback_t OnConnect, state_callback_t OnDisconnect, read_callback_t OnRead)
{
    std::lock_guard<std::mutex> locker(_lock);

    on_connect    = OnConnect;
    on_disconnect = OnDisconnect;
    on_read       = OnRead;
}


