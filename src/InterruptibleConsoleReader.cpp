#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <csignal>
#include <vector>
#include <functional>
#include <netdb.h>
#include <mutex>
#include <unistd.h>
#include <string>
#include "InterruptibleConsoleReader.hpp"

InterruptibleConsoleReader::InterruptibleConsoleReader(void *ext_data, read_callback_t OnRead, int break_signal) 
    :_ext_data(ext_data),
    on_read(OnRead),
    read_thread(-1),
    _break_signal(break_signal)
{
}

int InterruptibleConsoleReader::WaitDataForReadInfinity(int socket_fd, sigset_t sig_mask)
{
    fd_set readfds;
    
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    int rc = pselect(socket_fd+1, &readfds, NULL, NULL, NULL, &sig_mask);
    return rc;
}

void InterruptibleConsoleReader::InterruptibleConsoleReaderSignalStopper(int signo)
{
    (void)signo;
}

void *InterruptibleConsoleReader::ReadThreadFunction()
{
    int rc = 0;

    sigset_t sigset, oldset;
    sigemptyset(&sigset);
    sigemptyset(&oldset);    
    sigaddset(&sigset, _break_signal );
    signal(_break_signal, InterruptibleConsoleReaderSignalStopper);
    sigprocmask(SIG_BLOCK, &sigset, &oldset);  

    std::string in_str;
    while (!stop_read)
    {
        rc = WaitDataForReadInfinity(stdin_fd, oldset);
        if (rc >= 0)
        {
            std::cin >> in_str;
            if (on_read)
            {
                if (on_read(_ext_data, in_str)){
                    break;
                }
            }
            in_str.clear();
        }
    }

    return NULL;
}

void *InterruptibleConsoleReader::StaticReadThreadFunction(void *_ctx)
{
    InterruptibleConsoleReader *ctx = (InterruptibleConsoleReader *)_ctx;  
    pthread_exit(ctx->ReadThreadFunction());
    // ctx->read_thread = (pthread_t)-1;
}

int InterruptibleConsoleReader::StartRead()
{
    if (read_thread != (pthread_t)-1){
        return -EEXIST;
    }

    stdin_fd = fileno(stdin);
    stop_read = false;
    return pthread_create(&read_thread, NULL, &StaticReadThreadFunction, this);
}

void InterruptibleConsoleReader::StopRead()
{
    /* only stop thread disconnect in thread */
    stop_read = true;

    if (read_thread != (pthread_t)-1){
        pthread_kill(read_thread, _break_signal); 
    }
}

void InterruptibleConsoleReader::WaitTerminate()
{
    if (read_thread != (pthread_t)-1){
        pthread_join(read_thread, NULL); 
    }
}

