#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

// How to set this up carefully?
// What parts should be done inthe constructor, and which are for afterwards?
// NOTE That the accept()  call is likely to have be outside
// FIXED How do I set up the _serverAddress sockaddr_in struct in the init list?
Server::Server(int port, std::string password) : _socketFD(0), _epollFD(0), _serverAddress(), _password(password)
{
	std::cout << "Server constructor with parameters called" << std::endl;
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFD == -1)
	{
		std::cerr << "Socket creation failed!" << std::endl;
		throw std::runtime_error("Socket creation failed");
	}
	std::cout << "Created a socket listening at fd " << _socketFD << std::endl;

	// Set socket to non-blocking
	int flags = fcntl(_socketFD, F_GETFL, 0);
	fcntl(_socketFD, F_SETFL, flags | O_NONBLOCK);

	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	std::cout << "Binding...";
	if (bind(_socketFD, (struct sockaddr *)&_serverAddress, sizeof(_serverAddress)) == -1) 
	{
		std::cerr << "Binding failed!" << std::endl;
		close(_socketFD);
		throw std::runtime_error("Binding failed");
	}
	std::cout << " Socket successfully bound" << std::endl;

	std::cout << "Listening..." << std::endl;
	if (listen(_socketFD, 5) == -1) 
	{
		std::cerr << "Listening failed!" << std::endl;
		close(_socketFD);
		throw std::runtime_error("Listening failed");
	}
	std::cout << "Server ready to accept connections." << std::endl;

	// Crear epoll
	_epollFD = epoll_create1(0);
	if (_epollFD == -1) 
	{
		std::cerr << "epoll_create1 failed!" << std::endl;
		close(_socketFD);
		throw std::runtime_error("epoll_create1 failed");
	}

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = _socketFD;
	if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &ev) == -1) 
	{
		std::cerr << "epoll_ctl failed!" << std::endl;
		close(_socketFD);
		close(_epollFD);
		throw std::runtime_error("epoll_ctl failed");
	}
}

// Método para aceptar clientes (bloqueante)
int Server::acceptClient()
{
	int clientSocket = accept(_socketFD, 0, 0);
	if (clientSocket == -1)
		std::cerr << "Accept failed!" << std::endl;
	else
		std::cout << "Accepted a connection, client fd: " << clientSocket << std::endl;
	return clientSocket;
}

void Server::run()
{
	std::cout << "Servidor en ejecución. Esperando conexiones (epoll)..." << std::endl;
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
				std::cout << "Nuevo cliente conectado, fd: " << clientSocket << std::endl;
				// Hacer el socket del cliente no bloqueante
				int flags = fcntl(clientSocket, F_GETFL, 0);
				fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
				// Añadir el cliente a epoll
				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = clientSocket;
				if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
				{
					std::cerr << "epoll_ctl add client failed!" << std::endl;
					close(clientSocket);
				}
			}
			else
			{
				// Hay datos para leer de un cliente
				char buf[512];
				int count = read(events[i].data.fd, buf, sizeof(buf) - 1);
				if (count <= 0)
				{
					std::cout << "Cliente desconectado, fd: " << events[i].data.fd << std::endl;
					close(events[i].data.fd);
					epoll_ctl(_epollFD, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				}
				else
				{
					buf[count] = '\0';
					std::cout << "Mensaje recibido de fd " << events[i].data.fd << ": " << buf << std::endl;
					// Aquí puedes procesar el mensaje recibido
				}
			}
		}
	}
}

// NOTE This needs to be expanded as we add more things to the Server class
Server::~Server(void)
{
	std::cout << "Server being taken down! Make sure all memory is properly deallocated" << std::endl;
	if (_socketFD > 0)
	{
		close(_socketFD);
		std::cout << "Server socket closed." << std::endl;
	}
	if (_epollFD > 0)
	{
		close(_epollFD);
		std::cout << "epoll fd closed." << std::endl;
	}
}

// Simple gettter for the Server socket's file descriptor.
int	Server::get_fd(void) const
{
	return (this->_socketFD);
}
