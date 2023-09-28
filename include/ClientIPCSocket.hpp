#ifndef __CLIENT_IPC_SOCKET_HPP__
#define __CLIENT_IPC_SOCKET_HPP__

class ClientIPCSocket
{  
    public:
        typedef std::function<void(void *, ClientIPCSocket &)> state_callback_t;
        typedef std::function<void(void *, ClientIPCSocket &, std::vector<uint8_t> &)> read_callback_t;

    private:
        int                socket_fd;
        struct sockaddr_un un_addr;

        static const size_t recive_buf_len = 64;
        uint8_t recive_buf[recive_buf_len];

        state_callback_t on_connect;
        state_callback_t on_disconnect;
        read_callback_t  on_read;

        int  thread_interrupt_signal;
        bool stop_read;
        pthread_t read_thread;
        void *ext_data;
        std::mutex _lock;
     
        void SocketClose();
        int WaitDataForReciveInfinity(int socket_fd, sigset_t sig_mask);
        int DataForReciveExists(int socket_fd);
        int Recive(std::vector<uint8_t> &buf, sigset_t sig_mask);

        static void ClientIPCSocketSignalStopper(int signo);
        void *ReadThreadFunction();
        static void *StaticReadThreadFunction(void *ctx);  

    public:
        ClientIPCSocket();
        ClientIPCSocket(int fd, struct sockaddr_un &addr);
        ~ClientIPCSocket();

        int Send(std::vector<uint8_t> &buf);

        int  Open();
        int  Connect(const std::string sock_path);
        int  StartReading();

        void Close();

        /* for call backs */
        void SetExtData(void *_ext_data);
        void SetCallbacs(state_callback_t OnConnect, state_callback_t OnDisconnect, read_callback_t OnRead); 
};

#endif /* __CLIENT_IPC_SOCKET_HPP__ */