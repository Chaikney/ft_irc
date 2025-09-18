#include <set>
#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <sys/epoll.h>
#include <fcntl.h>	// NOTE IS there a C++ equivalent we should prefer? 
					// RESPONSE No, fcntl is what we need to use because we cant use external libraries and stl has nothing better or equal.
// _clients debe ser miembro de la clase Server, no global
// Set up the Server:
// - create fd for socket
// - set as non-blocking
// - set to listen on IP4, user-supplied port and any incoming IP
// - bind socket to the fd
// - listen on fd
// - Create epoll fd
// - Add socket fd to epoll's listening set
// FIXED There are no try/catch blocks for the thrown exceptions

// Helper para poner un socket en modo no bloqueante
bool setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) return false;
	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

Server::Server(int port, std::string password) : _socketFD(0), _epollFD(0), _serverAddress(), _password(password), _clients()
{
	std::cout << "Server constructor with parameters called" << std::endl;
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFD == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	std::cout << "Created a socket listening at fd " << _socketFD << std::endl;

	// Set socket to non-blocking by:
	// Get the existing flags for the newly-created Socket
	int flags = fcntl(_socketFD, F_GETFL, 0);
	// Add non-blocking to those existing client flags
	fcntl(_socketFD, F_SETFL, flags | O_NONBLOCK);

	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	std::cout << "Binding...";
	if (bind(_socketFD, (struct sockaddr *)&_serverAddress, sizeof(_serverAddress)) == -1) 
	{
		close(_socketFD);
		throw std::runtime_error("Binding failed");
	}
	std::cout << " Socket successfully bound" << std::endl;

	std::cout << "Listening..." << std::endl;
	if (listen(_socketFD, 5) == -1) 
	{
		close(_socketFD);
		throw std::runtime_error("Listening set up failed");
	}
	std::cout << "Server ready to accept connections." << std::endl;

	// Crear epoll
	_epollFD = epoll_create1(0);
	if (_epollFD == -1)
	{
		close(_socketFD);
		throw std::runtime_error("Could not create epoll");
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = _socketFD;
	if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &ev) == -1) 
	{
		close(_socketFD);
		close(_epollFD);
		throw std::runtime_error("Could not add server input to epoll set.");
	}
}

// Método para aceptar clientes (bloqueante)
// NOTE This is unused right now
int Server::acceptClient()
{
	int clientSocket = accept(_socketFD, 0, 0);
	if (clientSocket == -1)
		std::cerr << "Accept failed!" << std::endl;
	else
		std::cout << "Accepted a connection, client fd: " << clientSocket << std::endl;
	return clientSocket;
}

// Activates the Server's epoll loop
// - Waits for an event
// -- if the event is on the Server fd, we treat it as a new connection
// --- i.e. we add it to the set of Things Listened For
// --- first setting it as nonblocking
// -- if it is *not* on the server fd then it is from a client
// --- TODO Do more useful things with the client input - build command, register users
// --- (Currently we just echo the input to stdout)
// TODO Some parts of this should be made more C++ like,
// e.g. stringstream insteaad of manually terminating a character buffer
void Server::run()
{
	std::stringstream ss;
	ss << "server on, waiting for conections (epoll)...";
	std::string msg = ss.str();
	std::cout << msg << std::endl;
	const int MAX_EVENTS = 10;
	struct epoll_event events[MAX_EVENTS];
	while (true)
	{
		int n = epoll_wait(_epollFD, events, MAX_EVENTS, -1);
		if (n == -1)
		{
			std::cerr << "epoll_wait error" << std::endl;
			break;
		}
		for (int i = 0; i < n; ++i)
		{
			if (events[i].data.fd == _socketFD)
			{
				// Nueva conexión entrante
				int clientSocket = accept(_socketFD, NULL, NULL);
				if (clientSocket == -1)
				{
					std::cerr << "Accept failed!" << std::endl;
					continue;
				}
				std::stringstream ss;
				ss << "New client connected, fd: " << clientSocket;
				std::string msg = ss.str();
				std::cout << msg << std::endl;
				if (!setNonBlocking(clientSocket))
				{
					std::cerr << "Failed to set client socket non-blocking!" << std::endl;
					close(clientSocket);
					continue;
				}
				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = clientSocket;
				if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
				{
					std::cerr << "epoll_ctl add client failed!" << std::endl;
					close(clientSocket);
				}
				else
				{
					_clients.insert(clientSocket);
				}
			}
			else
			{
				// Hay datos para leer de un cliente
				char buf[512];
				int count = read(events[i].data.fd, buf, sizeof(buf) - 1);
				if (count <= 0)
				{
					std::stringstream ss;
					ss << "Client disconected, fd: " << events[i].data.fd;
					std::string msg = ss.str();
					std::cout << msg << std::endl;
					close(events[i].data.fd);
					epoll_ctl(_epollFD, EPOLL_CTL_DEL, events[i].data.fd, NULL);
					_clients.erase(events[i].data.fd);
				}
				else
				{
					buf[count] = '\0';
					std::string msg(buf);
					std::stringstream ss;
					ss << "message received from fd " << events[i].data.fd << ": " << msg;
					std::string out = ss.str();
					std::cout << out << std::endl;
					// Reenviar el mensaje a todos los demás clientes
					for (std::set<int>::iterator it = _clients.begin(); it != _clients.end(); ++it)
					{
						if (*it != events[i].data.fd)
						{
							write(*it, msg.c_str(), msg.size());
						}
					}
				}
			}
		}
	}
}

// Simple gettter for the Server socket's file descriptor.
int	Server::get_fd(void) const
{
	return (this->_socketFD);
}

Server::~Server(void)
{
	// Libera recursos si es necesario
	std::cout << "Server destructor called." << std::endl;
}