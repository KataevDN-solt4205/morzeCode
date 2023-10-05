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
#include <termios.h>
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

    struct termios tp, save;

    /* Retrieve current terminal settings, turn echoing off */
    if (tcgetattr(STDIN_FILENO, &tp) == -1){
        return NULL;
    }

    /* save settings */
    save = tp;                          /* So we can restore settings later */

    /* ECHO off, other bits unchanged */
    tp.c_lflag &= ~ECHO;              

    /* Disable canonical mode, and set buffer size to 1 byte */
    tp.c_lflag &= (~ICANON);
    tp.c_cc[VTIME] = 0;
    tp.c_cc[VMIN] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &tp) == -1){
        return NULL;
    }

    std::string in_str;
    char c; 
    bool is_space = true;
    while (!stop_read)
    {
        rc = WaitDataForReadInfinity(stdin_fd, oldset);
        if (rc >= 0)
        {
            c = getchar();
            if ((c >= 'a') && (c <= 'z'))
            {                
                in_str += c;
                std::cout << c  << std::flush;
                is_space = false;

            }
            /* 1 delim between words */
            if ((c == ' ') && (is_space == false)){
                in_str += c;
                std::cout << c  << std::flush;
                is_space = true;
            }

            if ((c == 0x0A)){
                std::cout << std::endl;
                is_space = false;
            }

            if ((c == 0x0A) && (in_str.length() > 0 ))
            {
                if (on_read)
                {
                    if (on_read(_ext_data, in_str)){
                        break;
                    }
                }
                in_str.clear();
                is_space = true;
            }
        }
    }
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &save) == -1){
        return NULL;
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

