#ifndef __INTERRUPTIBLE_CONSOLE_READER_HPP__
#define __INTERRUPTIBLE_CONSOLE_READER_HPP__

class InterruptibleConsoleReader
{
    public:
        typedef std::function<bool(void *, std::string &)> read_callback_t;

    private:
        void *_ext_data;
        read_callback_t on_read;

        bool stop_read;
        pthread_t read_thread;
        int _break_signal;
        int stdin_fd;

        int WaitDataForReadInfinity(int socket_fd, sigset_t sig_mask);

        void *ReadThreadFunction();
        static void InterruptibleConsoleReaderSignalStopper(int signo);
        static void *StaticReadThreadFunction(void *_ctx);

    public:
        /* OnRead if return false then terminate */
        InterruptibleConsoleReader(void *ext_data, read_callback_t OnRead, int break_signal);
        int StartRead();
        void StopRead();
        void WaitTerminate();
};

#endif /* __INTERRUPTIBLE_CONSOLE_READER_HPP__ */