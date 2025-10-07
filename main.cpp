#include "Message.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <cstdlib>
#include <sstream>


// Parse command line arguments
// - port number (1-65535)
// - password (non-empty, valid characters)
// Create a Server instance with those arguments
int	main(int argc, char **argv)
{
	std::string password;
	int port_num;
	try
	{
		if (argc < 3)
			throw std::invalid_argument("Not enough parameters");
		//haciendo isstringstream comprobamos automaticamente posibles errores de conversion y !portav >> port_num lo convierte a int
		std::istringstream port_av(argv[1]);
		if (!(port_av >> port_num) || port_num <= 0 || port_num > 65535)
			throw std::invalid_argument("Invalid port number");
		password = argv[2];
		if (password.empty())
			throw std::invalid_argument("Password cannot be empty");
		if (password.length() > 32)
			throw std::invalid_argument("Password is longer than 32 characters.");
		
		// Security: Enhanced password validation
		for (size_t i = 0; i < password.length(); ++i)
		{
			char c = password[i];
			// Allow only printable ASCII characters except forbidden ones
			if (c < 33 || c > 126 || c == ',' || c == ':' || c == '\\' || c == '\'' || c == '"' || c == ' ')
				throw std::invalid_argument("Password contains invalid character");
		}
		std::cout << "calling server constructor" << std::endl;
		Server	ircServer(port_num, password);
		ircServer.run();
	}
	catch (std::invalid_argument &e)
	{
		std::stringstream ss;
		ss << "Problem with starting parameters: " << e.what();
		std::cerr << ss.str() << std::endl;
		// Si tuvieras un log, podrías reutilizar el mensaje:
		// logFile << ss.str() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		exit (EXIT_FAILURE);
	}
	catch (std::runtime_error &e)
	{
		std::stringstream ss;
		ss << "Unable to start up Server: " << e.what();
		std::cerr << ss.str() << std::endl;
		// logFile << ss.str() << std::endl;
		exit (EXIT_FAILURE);
	}
    return (0);
}
