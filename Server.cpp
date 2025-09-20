#include "Server.hpp"
#include "Message.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>	// NOTE IS there a C++ equivalent we should prefer?
#include <sstream>
#include <queue>
#include <cstdlib>	// for the EXIT code
#include <cstring>	// for memset. Too many includes!

// Set up the Server:
// - create fd for socket
// - set as non-blocking
// - set to listen on IP4, user-supplied port and any incoming IP
// - bind socket to the fd
// - listen on fd
// - Create epoll fd
// - Add socket fd to epoll's listening set
// FIXED There are no try/catch blocks for the thrown exceptions
Server::Server(int port, std::string password) : _socketFD(0), _epollFD(0),
												 _serverAddress(), _password(password),
												 _toProcess()
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
				// Get the existing flags for the newly-accepted clientSocket
				int flags = fcntl(clientSocket, F_GETFL, 0);
				// Add non-blocking to those existing client flags
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
				// We read one less than buffer size so we can add the null char later
				int count = read(events[i].data.fd, buf, sizeof(buf) - 1);
				if (count <= 0)
				{
					std::cout << "Cliente desconectado, fd: " << events[i].data.fd << std::endl;
					close(events[i].data.fd);
					epoll_ctl(_epollFD, EPOLL_CTL_DEL, events[i].data.fd, NULL);
				}
				else	// There is input to manage
				{
					// NOTE There is apparently no sensible way to do this
					// We have to go char buf-string-stringstream :|
					// We need to see whether we have enough for a complete message
					// And then put it in the queue
//					std::string	tmp;
					// FIXME Make sure that buf is not NULL before passing to the string constructor
					std::string	str_buf(buf);
					if (str_buf.empty() == false)	// HACK this codde is disgusting
						std::cout << "Our string is: " << str_buf << std::endl;
					std::stringstream	strm_msg(str_buf);
//					buf[count] = '\0';
//					NOTE This check does not work.
					// std::getline(strm_msg, tmp, '\r');
					// if (strm_msg.peek() == '\n')
					// this is a complete messsage we can do something with it
					// TODO Also add the  \n, OR remnove the \r as we don't need it now
					try
					{
						// NOTE This *should* be the cut-till crlf tmp string though
						Message	*nxtMessage = Message::makeMessage(str_buf);
						std::cout << nxtMessage << std::endl;
						this->_toProcess.push(nxtMessage);
					}
					catch (std::exception &e)
					{
						std::cerr << "Something wrong in message queuing. " << std::endl;
						std::cerr << e.what() <<std::endl;
						exit (EXIT_FAILURE);
					}
					// Need to clear the buffer BUT really should be storing / running until crlf
					memset(buf, ' ', 511);
					buf[512] = '\0';
// 					else
// 					{
// 						std::cout << "Partial message discarded because not there yet" << std::endl;
// 						// TODO store the partial message for later?
// 					}
					std::cout << "Mensaje recibido de fd " << events[i].data.fd << ": " << buf << std::endl;
					std::cout << "Printing queued messages" << std::endl;
					// TODO We need something to check that the message is complete
					this->_printMessageQueue(this->_toProcess);
					// ...or a place to store it if it is not.
					// Move the Message to a processing queue
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

// Debug function to view a message queue.
// Uses a copy not a reference so that we don't lose item
// (Yes that is probably *very* inefficient)
void	Server::_printMessageQueue(std::queue<Message *> toPrint)
{
	Message	*this_one;
	int	n;

	n = toPrint.size();
	std::cout << "Printing message queue with " << n << " items" << std::endl;
	while (toPrint.empty() != true)
	{
		this_one = toPrint.front();
		std::cout << this_one;
		toPrint.pop();
	}
	std::cout << n << "Messages printed" << std::endl;
}
