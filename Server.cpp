#include "Server.hpp"
#include <unistd.h>     // close()
#include <cstring>      // std::memset, std::strerror
#include <cerrno>       // errno
#include <arpa/inet.h>  // htons
#include <sys/types.h>
#include <unistd.h>

Server::Server(unsigned short port, int backlog) : _sockfd(-1) , _addr(), _backlog(backlog)
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd == -1)
        throw std::runtime_error("socket() failed: " + std::string(std::strerror(errno)));

    int opt = 1;
    if (setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(_sockfd);
        throw std::runtime_error("setsockopt() failed: " + std::string(std::strerror(errno)));
    }

    // ensure zeroed before setting fields (redundant with _addr() but safe)
    std::memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_sockfd, reinterpret_cast<struct sockaddr*>(&_addr), sizeof(_addr)) == -1)
    {
        close(_sockfd);
        throw std::runtime_error("bind() failed: " + std::string(std::strerror(errno)));
    }

    if (listen(_sockfd, _backlog) == -1)
    {
        close(_sockfd);
        throw std::runtime_error("listen() failed: " + std::string(std::strerror(errno)));
    }
}

Server::~Server()
{
    if (_sockfd != -1)
        close(_sockfd);
}

int Server::acceptClient()
{
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    int client = accept(_sockfd, reinterpret_cast<struct sockaddr*>(&clientAddr), &len);
    if (client == -1)
        throw std::runtime_error("accept() failed: " + std::string(std::strerror(errno)));
    return client;
}

int Server::fd() const 
{
    return _sockfd;
}