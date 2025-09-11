#include <iostream>
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct

// TODO The server set up should be part of a Server class
// TODO Make use of the errnos in case of failure - exceptions?
// TODO How do test the accept() ed client?
// (See man 2 accept - this call blocks by default)
// TODO The second argument of listen should be a constant in a header (QUEUESIZE or similiar)
int	main(void)
{
	sockaddr_in	serverAddress;
	int	socket_fd;
	int	bind_status;
	int	clientSocket;

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
		std::cerr << "Socket creation failed!" << std::endl;
	else
		std::cout << "Created a socket listening at fd " << socket_fd <<std::endl;
	serverAddress.sin_family = AF_INET;	// say what type of socket it is
	serverAddress.sin_port = htons(6667);	// listen to IRC port; htons handles a conversion
	serverAddress.sin_addr.s_addr = INADDR_ANY;	// listens on any available IP
	// "Assign a name to the socket"
	std::cout << "binding....";
	bind_status = bind(socket_fd, (struct sockaddr *) &serverAddress, sizeof(serverAddress));
	if (bind_status == -1)
		std::cerr << "Binding failed!" << std::endl;
	else
		std::cout << "Socket successfully bound" << std::endl;
	std::cout << "listening....." << std::endl;;
	if (listen(socket_fd, 5) == -1)
		std::cerr << "listening failed!" << std::endl;
	std::cout << "ready to accept....." << std::endl;;
	clientSocket = accept(socket_fd, 0, 0);
	std::cout << "Asked for a connection and got " << clientSocket << std::endl;
    return (0);
}
