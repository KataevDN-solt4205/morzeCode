#ifndef __INTERRUPTIBLE_CONSOLE_READER_HPP__
#define __INTERRUPTIBLE_CONSOLE_READER_HPP__

class InterruptibleConsoleReader : public BasicReadBufferSupervisor
{
    public:
        typedef std::function<bool(void *, std::string &)> read_callback_t;

    private:
        void *_ext_data;
        read_callback_t on_read;
        struct termios termios_old;
        std::string in_str;
        bool is_space;

        int  BeforeThreadLoop();
        int  ExistDataInReadBuffer();
        void AfterThreadLoop();

    public:
        /* OnRead if return false then terminate */
        InterruptibleConsoleReader(void *ext_data, read_callback_t OnRead);
};

#endif /* __INTERRUPTIBLE_CONSOLE_READER_HPP__ */