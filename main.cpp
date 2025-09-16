#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <cstdlib>
// TO DO parse command line arguments
// - port number
// - password
// Check that port number is valid (1-65535)
// Check that password is non-empty
// Create a Server instance with those arguments
// TODO Add signal handlers to cleanly exit on SIGINT and SIGTERM
// TODO Add error handling for invalid arguments
// TODO Add error handling for Server constructor problems
int	main(int ac, char **av)
{
	int port;
	std::string password;

	port = -1;
	if(ac < 3)
	{
		std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
		return (1);
	}
	port = atoi(av[1]);
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Invalid port number" << std::endl;
		return (1);
	}
	password = av[2];

	std::cout << "calling server constructor" << std::endl;
	Server	ircServer(port, password);
    return (0);
}
