#ifndef __BASIC_READ_BUFFER_SUPERVISOT_HPP__
#define __BASIC_READ_BUFFER_SUPERVISOT_HPP__

class BasicReadBufferSupervisor
{
    protected:
        int fd;
        bool stop_thread;
        pthread_t thread;
        std::mutex thread_lock;
        bool is_running;
        const size_t check_delay_in_sec = 0;
        const size_t check_delay_in_usec = 500;

    protected:
        int WaitDataInReadBuf(size_t sec, size_t usec);
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
        BasicReadBufferSupervisor();
        virtual ~BasicReadBufferSupervisor();

        int  Start();
        void Stop();
        void Join();
        bool IsRunning();

        virtual void Attach(int _fd);
        virtual void Detach();

};

#endif /* __BASIC_READ_BUFFER_SUPERVISOT_HPP__ */