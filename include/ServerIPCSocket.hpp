#ifndef __SERVER_IPC_SOCKET_HPP__
#define __SERVER_IPC_SOCKET_HPP__

class ServerIPCSocket: public BasicReadBufferSupervisor
{  
    private:
        const int backlog = 5;

        void *ext_data;
        std::mutex list_lock;
        size_t max_clients;

        std::vector<ClientIPCSocket*> clients;
        std::vector<ClientIPCSocket*> deleted_clients;

    public:
        typedef std::function<void(void *, int, ClientIPCSocket *)> state_callback_t;

    private:
        state_callback_t on_connect;
        state_callback_t on_disconnect;

    public:
        ServerIPCSocket(void *_ext_data, state_callback_t OnConnect, state_callback_t OnDisconnect);
        ~ServerIPCSocket();

        int  Open();
        void Close();
        int  Bind(const std::string uri);
        int  Listen(size_t _max_clients);

        int Send(std::vector<uint8_t> &buf);

    private:        
        int Accept(int &client_fd);
        void FreeDeletedClient();
        static void OnClientDisconnect(void *ext_data, ClientIPCSocket &client);          
        int ExistDataInReadBuffer();
};

#endif /* __SERVER_IPC_SOCKET_HPP__ */