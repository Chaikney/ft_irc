#include "Message.hpp"
#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <cstdlib>
#include <sstream>


// HACK temp test "suite" for Message creation
void	runMessageParsingTests(void)
{
	Message	*test_msg;
	std::string	test_str;

	test_str = "NICK pants and socks";
	test_msg = Message::makeMessage(test_str);
	std::cout << *test_msg << std::endl;
	delete test_msg;
	test_str = "    ";
	test_msg = Message::makeMessage(test_str);
	std::cout << test_msg << std::endl;
	delete test_msg;
	test_str = (":not valid is it");
	test_msg = Message::makeMessage(test_str);
	std::cout << *test_msg << std::endl;
	delete test_msg;
	test_str = ("@tag :source command and then a long list of parameters");
	test_msg = Message::makeMessage(test_str);
	std::cout << *test_msg << std::endl;
	delete test_msg;
	test_str = ("@tag :source command :and then a long list of parameters treated as one");
	test_msg = Message::makeMessage(test_str);
	std::cout << *test_msg << std::endl;
	delete test_msg;
}


// TO DO parse command line arguments
// - port number
// - password
// Check that port number is valid (1-65535)
// Check that password is non-empty
// Create a Server instance with those arguments
// TODO Add signal handlers to cleanly exit on SIGINT and SIGTERM
// DONE Add error handling for invalid arguments
// DONE Add error handling for Server constructor problems
int	main(int argc, char **argv)
{
	std::string password;
	int port_num;
	runMessageParsingTests();	// HACK for debugging remove later
//	exit(EXIT_SUCCESS);
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
		for (size_t i = 0; i < password.length(); ++i)
		{
			if (password[i] < 33 || password[i] > 126 || password[i] == 44 || password[i] == 58 || password[i] == 92 || password[i] == 39 || password[i] == 34)
				throw std::invalid_argument("Password contains invalid character");
		}
		std::cout << "calling server constructor" << std::endl;
		Server	ircServer(port_num, password);
		ircServer.run();
	}
	catch (std::invalid_argument &e)
	{
		std::cerr << "Problem with statrting parameters: " << e.what() << std::endl;
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		exit (EXIT_FAILURE);
	}
	catch (std::runtime_error &e)
	{
		std::cerr << "Unable to start up Server: " << e.what() << std::endl;
		exit (EXIT_FAILURE);
	}
    return (0);
}
