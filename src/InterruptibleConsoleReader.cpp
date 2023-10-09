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
#include "BasicReadBufferSupervisor.hpp"
#include "InterruptibleConsoleReader.hpp"

InterruptibleConsoleReader::InterruptibleConsoleReader(void *ext_data, read_callback_t OnRead) 
    :BasicReadBufferSupervisor(),
    _ext_data(ext_data),
    on_read(OnRead)
{
}

int InterruptibleConsoleReader::BeforeThreadLoop()
{
    struct termios tp;

    /* Retrieve current terminal settings, turn echoing off */
    if (tcgetattr(fd, &tp) == -1){
        return -errno;
    }

    /* save settings */
    termios_old = tp;                         

    /* ECHO off, other bits unchanged */
    tp.c_lflag &= ~ECHO;              

    /* Disable canonical mode, and set buffer size to 1 byte */
    tp.c_lflag &= (~ICANON);
    tp.c_cc[VTIME] = 0;
    tp.c_cc[VMIN] = 1;

    if (tcsetattr(fd, TCSAFLUSH, &tp) == -1){
        return -errno;
    }

    return 0;
}

int InterruptibleConsoleReader::ExistDataInReadBuffer()
{
    char c = getchar();   

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
                return -1;
            }
        }
        in_str.clear();
        is_space = false;
    }

    return 0;
}

void InterruptibleConsoleReader::AfterThreadLoop()
{
    tcsetattr(fd, TCSAFLUSH, &termios_old);
}


