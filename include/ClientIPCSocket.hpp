#ifndef __CLIENT_IPC_SOCKET_HPP__
#define __CLIENT_IPC_SOCKET_HPP__

class ClientIPCSocket : public BasicReadBufferSupervisor
{  
    public:
        typedef std::function<void(void *, ClientIPCSocket &)> state_callback_t;
        typedef std::function<void(void *, ClientIPCSocket &, std::vector<uint8_t> &)> read_callback_t;

    private:
        static const size_t recive_buf_len = 64;
        uint8_t recive_buf[recive_buf_len];

        state_callback_t on_connect;
        state_callback_t on_disconnect;
        read_callback_t  on_read;

        void *ext_data;

        std::mutex ext_data_and_callbaks_lock;

        std::vector<uint8_t> read_buf;

    public:
        ClientIPCSocket(void *_ext_data, 
            state_callback_t OnConnect, state_callback_t OnDisconnect, read_callback_t OnRead);
        ClientIPCSocket(int fd, void *_ext_data, 
            state_callback_t OnConnect, state_callback_t OnDisconnect, read_callback_t OnRead);
        ~ClientIPCSocket();

        void SetExtData(void *_ext_data);
        void SetOnConnectCallback(state_callback_t OnConnect);
        void SetOnDisconnectCallback(state_callback_t OnDisconnect);
        void SetOnReadCallback(read_callback_t OnRead);

        int Send(std::vector<uint8_t> &buf);

        int  Open();
        void Close();
        int  Connect(const std::string sock_path);

    private:
        int BeforeThreadLoop();
        int ExistDataInReadBuffer();
        void AfterThreadLoop();
};

#endif /* __CLIENT_IPC_SOCKET_HPP__ */