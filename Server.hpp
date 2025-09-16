#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <stdexcept>

class Server
{
    public:
        explicit Server(unsigned short port, int backlog = 5);
        ~Server();

        int acceptClient();
        int fd() const;

    private:
        int _sockfd;
        struct sockaddr_in _addr;
        int _backlog;

        Server(const Server &);
        Server &operator=(const Server &);
};// SERVER_HPP