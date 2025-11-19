#include "Message.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <cstdlib>
#include <sstream>


// Check and return the port number. Will throw exception on bad input
//haciendo isstringstream comprobamos automaticamente posibles errores de conversion y !portav >> port_num lo convierte a int
int	getPortNumber(char *argv1)
{
	int	port_num;
	std::istringstream port_av(argv1);
	if (!(port_av >> port_num))
		throw std::invalid_argument("Could not read first parameter as number");
	else if (port_num <= 0 || port_num > 65535)
		throw std::invalid_argument("Port number is out of range");
	else if (port_num <= 1024)
		throw std::invalid_argument("Port numbers of 1024 and below are reserved for system use");
	return (port_num);
}

// Check and return the password. Will throw exception on bad input
// DONE (mostly) Use C++ idioms when checking for invalid characters in the password
// 44 = , 58 = : 92 = \ 34 = " 39 = '
// ...these prohibitions seem sensible except the comma, what is the problem there?
std::string	getPassword(char *argv2)
{
	std::string password(argv2);
	if (password.empty())
		throw std::invalid_argument("Password cannot be empty");
	if (password.length() > 32)
		throw std::invalid_argument("Password is longer than 32 characters.");
	for (size_t i = 0; i < password.length(); ++i)
	{
		if (password[i] < 33 || password[i] > 126)
			throw std::invalid_argument("Password char outwith printable ASCII range");
	}
	if (password.find_first_of("\\,:\'\"") != std::string::npos)
		throw std::invalid_argument("Password contains invalid character");
	return (password);
}

// Parse command line arguments
// - port number
// - password
// Check that port number is valid (1-65535)
// Check that password is non-empty, has no troublesome characters
// Create a Server instance with those arguments
// DONE Forbid system ports 0-1023
int	main(int argc, char **argv)
{
	std::string password;
	int port_num;
	try
	{
		if (argc < 3)
			throw std::invalid_argument("Not enough parameters");
		port_num = getPortNumber(argv[1]);
		password = getPassword(argv[2]);
		std::cout << "calling server constructor" << std::endl;
		Server	ircServer(port_num, password);
		ircServer.run();
	}
	catch (std::invalid_argument &e)
	{
		std::cerr << "Problem with starting parameters: " << e.what() << std::endl;
		if (argc >= 3)
			std::cerr << "I received:\tPort:" << argv[1] << "\tPassword:" << argv[2] << std::endl;
		std::cerr << "Usage:\t" << argv[0] << " <port> <password>" << std::endl;
	}
	catch (std::runtime_error &e)
	{
		std::cerr << "Unable to start up Server: " << e.what() << std::endl;
	}
    return (0);
}
