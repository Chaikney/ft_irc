#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>
#include <queue>// Messages to be processed
#include <set>	// FDs of clients to be sent to
#include <map>	// dictionary of partial messages

class	Message;

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
		std::queue<Message *>	_toProcess;
		std::set<int> _clients;		// NOTE This is the fds to be sent to; may duplicate other info
		std::map<int, std::string>	_partial_msgs;

					Server(void);	// private so not called
					Server(const Server &irc);	// No good reason to allow copy construction of the server
		Server		operator=(const Server &irc);	// No assignment should be possible either

		void		_printMessageQueue(std::queue<Message *> toPrint) const;
		void		_addNewClient();
		void		_removeClient(struct epoll_event &bye);
		bool 		_setNonBlocking(int fd);
		bool		_isFullMsg(std::string msg, int to_chk) const;
		void		_storePartial(int fd_source, std::string msg);
		std::string	_getClientInput(int fd);
		bool		_checkPass(Message &msg) const;
		void		_processQueue(void);

		// --- Manejo de comandos IRC ---
		void        handleKick(Message *msg, int sender_fd);
		void        handlePrivmsg(Message *msg, int sender_fd);

	public:
					Server(int port, std::string password);
					~Server(void);	// NOTE Destructor will have to cleanly close connections and whatever partial / pending messages we have

		int			get_fd(void) const;
		void		run(void);
};
#endif
