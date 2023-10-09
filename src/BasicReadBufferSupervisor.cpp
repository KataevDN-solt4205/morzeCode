#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <csignal>
#include <vector>
#include <functional>
#include <unistd.h>
#include <string>
#include <mutex>
#include <iostream>
#include "BasicReadBufferSupervisor.hpp"

BasicReadBufferSupervisor::BasicReadBufferSupervisor()
    :fd(-1),
    is_running(false)
{

}

BasicReadBufferSupervisor::~BasicReadBufferSupervisor()
{
}

int  BasicReadBufferSupervisor::WaitDataInReadBuf(size_t sec, size_t usec)
{
    fd_set readfds;
    struct timeval tv = {.tv_sec=0, .tv_usec=0};
       
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    int rc = select(fd+1, &readfds, NULL, NULL, &tv);
    return rc;
}

int  BasicReadBufferSupervisor::DataInReadBufExists()
{
    fd_set readfds;
    struct timeval tv = {.tv_sec=0, .tv_usec=0};
        
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    return select(fd+1, &readfds, NULL, NULL, &tv);
}

int  BasicReadBufferSupervisor::BeforeThreadLoop()
{
    return 0;
}

int  BasicReadBufferSupervisor::ExistDataInReadBuffer()
{
    return 0;
}

void BasicReadBufferSupervisor::AfterThreadLoop()
{

}

void BasicReadBufferSupervisor::SignalStopper(int signo)
{    
    (void)signo;
}

void *BasicReadBufferSupervisor::StaticThreadFunction(void *_ctx)
{
    BasicReadBufferSupervisor *ctx = (BasicReadBufferSupervisor *)_ctx;  
    pthread_exit(ctx->ThreadFunction());   
}

void *BasicReadBufferSupervisor::ThreadFunction()
{
    {
        std::lock_guard<std::mutex> Lock(thread_lock);
        is_running = true;
    } 

    if (BeforeThreadLoop()){
        stop_thread = true;
    }

    int data_exists; 
    do 
    {
        data_exists = WaitDataInReadBuf(check_delay_in_sec, check_delay_in_usec); 
        if (data_exists > 0)
        {
            if (ExistDataInReadBuffer()){
                break;
            }
        }
    }
    while ((stop_thread == false) && (data_exists >= 0));
 
    {
        std::lock_guard<std::mutex> Lock(thread_lock);
        is_running = false;
    }

    AfterThreadLoop();

    return NULL; 
}  

int BasicReadBufferSupervisor::Start()
{
    int rc = 0;
    if (fd == -1){
        return -EBADF;
    }

    {
        std::lock_guard<std::mutex> Lock(thread_lock);
        if (is_running){
            return -EEXIST;
        }
    }
    stop_thread = false;  
    rc = pthread_create(&thread, NULL, &StaticThreadFunction, this);
    /* wait thread started */
    int max_wait_count = 20; //2 sec
    while ((rc == 0) && (max_wait_count > 0))
    {
        if (is_running){
            break;
        }
        usleep(100); 
        max_wait_count--;
    }    
    return rc;
}

void BasicReadBufferSupervisor::Stop()
{
    stop_thread = true;    
}

void BasicReadBufferSupervisor::Join()
{
    bool local_is_running;
    {
        std::lock_guard<std::mutex> Lock(thread_lock);  
        local_is_running = is_running;
    }    

    if (local_is_running){             
        pthread_join(thread, NULL); 
    }
}

bool BasicReadBufferSupervisor::IsRunning()
{
    std::lock_guard<std::mutex> Lock(thread_lock);  
    return is_running;
}

void BasicReadBufferSupervisor::Attach(int _fd)
{
    fd = _fd;
}

void BasicReadBufferSupervisor::Detach()
{
    Stop();
    fd = -1;
}

