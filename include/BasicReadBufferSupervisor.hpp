#ifndef __BASIC_READ_BUFFER_SUPERVISOT_HPP__
#define __BASIC_READ_BUFFER_SUPERVISOT_HPP__

class BasicReadBufferSupervisor
{
    protected:
        int fd;
        int  thread_interrupt_signal;
        bool stop_thread;
        pthread_t thread;
        std::mutex thread_lock;
        bool is_running;
        sigset_t oldset;

    protected:
        int WaitDataInReadBufInfinity(sigset_t *oldset);
        int DataInReadBufExists();

        /* if ret != 0 thread terminate */
        virtual int  BeforeThreadLoop();
        /* if ret != 0 thread terminate */
        virtual int  ExistDataInReadBuffer();
        virtual void AfterThreadLoop();

    private:
        static void SignalStopper(int signo);
        static void *StaticThreadFunction(void *ctx);  
        void *ThreadFunction();

    public:
        BasicReadBufferSupervisor(int break_signal);
        virtual ~BasicReadBufferSupervisor();

        int  Start();
        void Stop();
        void Join();
        bool IsRunning();

        virtual void Attach(int _fd);
        virtual void Detach();

};

#endif /* __BASIC_READ_BUFFER_SUPERVISOT_HPP__ */