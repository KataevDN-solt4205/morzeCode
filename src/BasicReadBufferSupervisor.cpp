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

BasicReadBufferSupervisor::BasicReadBufferSupervisor(int break_signal)
    :fd(-1),
    thread_interrupt_signal(break_signal),
    is_running(false)
{

}

BasicReadBufferSupervisor::~BasicReadBufferSupervisor()
{
}

int  BasicReadBufferSupervisor::WaitDataInReadBufInfinity(sigset_t *oldset)
{
    fd_set readfds;
       
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    int rc = pselect(fd+1, &readfds, NULL, NULL, NULL, oldset);
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
    
    sigset_t sigset;
    sigemptyset(&sigset);
    sigemptyset(&oldset);    
    sigaddset(&sigset, thread_interrupt_signal );
    signal(thread_interrupt_signal, SignalStopper);
    sigprocmask(SIG_BLOCK, &sigset, &oldset); 

    if (BeforeThreadLoop()){
        stop_thread = true;
    }
      
    std::cout << "START "<< pthread_self() << std::endl;  
    int data_exists = WaitDataInReadBufInfinity(&oldset);  
    std::cout << "first wait exit"<< this << std::endl;
    while ((stop_thread == false) && (data_exists >= 0))
    {      
        if (ExistDataInReadBuffer()){
            break;
        }
        data_exists = WaitDataInReadBufInfinity(&oldset);
        std::cout << "second wait exit"<< this << std::endl;
    }
    
 
    {
        std::lock_guard<std::mutex> Lock(thread_lock);
        is_running = false;
    }

    AfterThreadLoop();
    // sigprocmask(SIG_UNBLOCK, &sigset, NULL);

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
    bool local_is_running;
    {
        std::cout << " Stop before thread lock"<< this  << " + " << thread <<std::endl;
        // std::lock_guard<std::mutex> Lock(thread_lock);  
        local_is_running = is_running;
    }    
    std::cout << "Stop after thread lock"<< this << " + " << is_running  << " + " << local_is_running << " + " << thread_interrupt_signal << std::endl;
    if (local_is_running){                     
        pthread_kill(thread, thread_interrupt_signal);
    }
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

