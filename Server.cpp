#include "Server.hpp"
#include <iostream>

// How to set this up carefully?
// What parts should be done inthe constructor, and which are for afterwards?
// NOTE That the accept()  call is likely to have be outside
// FIXED How do I set up the _serverAddress sockaddr_in struct in the init list?
Server::Server(int port, std::string password) :  _socketFD(0), _serverAddress(), _password(password)
{
    int	clientSocket;	// Probably does not belong here

    std::cout << "Server constructor with parameters called" << std::endl;
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFD == -1)
		std::cerr << "Socket creation failed!" << std::endl;
	else
		std::cout << "Created a socket listening at fd " << _socketFD <<std::endl;
	_serverAddress.sin_family = AF_INET;	// say what type of socket it is
	_serverAddress.sin_port = htons(port);	// listen to IRC port; htons handles a conversion
	_serverAddress.sin_addr.s_addr = INADDR_ANY;	// listens on any available IP
	// "Assign a name to the socket"
	std::cout << "binding....";
	if (bind(_socketFD, (struct sockaddr *) &_serverAddress,
             sizeof(_serverAddress)) == -1)
		std::cerr << "Binding failed!" << std::endl;
	else
		std::cout << "Socket successfully bound" << std::endl;
	std::cout << "listening....." << std::endl;;
	if (listen(_socketFD, 5) == -1)
		std::cerr << "listening failed!" << std::endl;
	std::cout << "ready to accept....." << std::endl;;
    // TODO Having this in the constructor means that everything else halts
    // ...is that OK, or does it cause problems?
	clientSocket = accept(_socketFD, 0, 0);
	std::cout << "Asked for a connection and got " << clientSocket << std::endl;

}

// NOTE This needs to be expanded as we add more things to the Server class
Server::~Server(void)
{
	std::cout << "Server being taken down! Make sure all memory is properly deallocated" << std::endl;
}
