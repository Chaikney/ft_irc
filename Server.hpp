
#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>

// What should this hold?
// - port
// - password (passed in on command line)
// TODO Add storage for Registered Users
// TODO Add storage for Channels (and members?)
// TODO Which attributes should be marked as const?
// TODO Add getters for any relevant attribute
class	Server
{
	private:
		int         _socketFD;
		int         _epollFD;
		sockaddr_in _serverAddress;
		std::string _password;

					Server(void);	// private so not called
					Server(const Server &irc);	// No good reason to allow copy construction of the server
		Server		operator=(const Server &irc);	// No assignment should be possible either

		int			acceptClient();

	public:
					Server(int port, std::string password);
					~Server(void);	// NOTE Destructor will have to cleanly close connections and whatever partial / pending messages we have

		int			get_fd(void) const;
		void		run(void);
};
#endif
