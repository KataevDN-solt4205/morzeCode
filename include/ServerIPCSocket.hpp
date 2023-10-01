#ifndef __SERVER_IPC_SOCKET_HPP__
#define __SERVER_IPC_SOCKET_HPP__

class ServerIPCSocket
{  
    private:
        int                socket_fd;
        struct sockaddr_un un_addr;

        int  thread_interrupt_signal;
        bool stop_accept;
        pthread_t accept_thread;
        void *ext_data;
        std::mutex _lock;
        size_t max_clients;
        std::vector<ClientIPCSocket*> clients;
     
        void SocketClose();
        int WaitDataForAcceptInfinity(int socket_fd, sigset_t sig_mask);
        int Accept(int &client_fd, struct sockaddr_un &client_sockaddr, sigset_t &sig_mask);

        static void OnClientConnect(void *ext_data, ClientIPCSocket &client);
        static void OnClientDisconnect(void *ext_data, ClientIPCSocket &client);          
        static void OnClientRead(void *ext_data, ClientIPCSocket &client, std::vector<uint8_t> &buf); 

        void *AcceptThreadFunction();
        static void *StaticAcceptThreadFunction(void *ctx); 

    public:
        typedef std::function<void(void *, int, ClientIPCSocket *)> state_callback_t;

    private:
        state_callback_t on_connect;
        state_callback_t on_disconnect;

    public:
        ServerIPCSocket();
        ~ServerIPCSocket();

        int  Open();
        int  Bind(const std::string sock_path);
        int  Listen(const int backlog);
        int  StartAcceptThread(const int max_clients);

        int SendAll(std::vector<uint8_t> &buf);

        void Close();
        void SetExtData(void *_ext_data);
        void SetCallback(state_callback_t OnConnect, state_callback_t OnDisconnect);
};

#endif /* __SERVER_IPC_SOCKET_HPP__ */