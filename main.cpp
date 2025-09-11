#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct

// TODO Read and check two parameters - port and password - to use in constructor
int	main(void)
{
	std::cout << "calling server constructor" << std::endl;
	Server	ircServer(6667, "");
    return (0);
}
