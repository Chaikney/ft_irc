#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <cstdlib>
#include <sstream>
// TO DO parse command line arguments
// - port number
// - password
// Check that port number is valid (1-65535)
// Check that password is non-empty
// Create a Server instance with those arguments
// TODO Add signal handlers to cleanly exit on SIGINT and SIGTERM
// TODO Add error handling for invalid arguments
// TODO Add error handling for Server constructor problems
int	main(int argc, char **argv)
{
	std::string password;
	int port_num;
	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return (1);
	}
	//haciendo isstringstream comprobamos automaticamente posibles errores de conversion y !portav >> port_num lo convierte a int
	std::istringstream port_av(argv[1]);
    if (!(port_av >> port_num) || port_num <= 0 || port_num > 65535)
    {
        std::cerr << "Invalid port number" << std::endl;
        return (1);
    }
	password = argv[2];
	if (password.empty())
	{
		std::cerr << "Password cannot be empty" << std::endl;
    	return 1;
	}
	if (password.length() > 32)
		std::cerr << "Password is longer than 32 characters." << std::endl;
	for (size_t i = 0; i < password.length(); ++i)
	{
    	if (password[i] < 33 || password[i] > 126 || password[i] == 44 || password[i] == 58 || password[i] == 92 || password[i] == 39 || password[i] == 34)
    	{
        	std::cerr << "Password contains invalid character or is longer than 32 characters " << std::endl;
        	return 1;
    	}
	}
	std::cout << "calling server constructor" << std::endl;
	Server	ircServer(port_num, password);
	ircServer.run();
    return (0);
}
