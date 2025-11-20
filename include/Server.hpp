#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>
#include <queue>// Messages to be processed
#include <map>	// dictionary of partial messages
#include <ctime>

class	Message;
class	User;
class	Channel;
class	ACommand;

const std::string	SERVERNAME	= "ft_irc";
const std::string	VERSION	= "0.9";	// NOTE Increment this when we get bored
const int			MAX_EVENTS = 10;

// What should this hold?
// - port
// - password (passed in on command line)
// IDEA A function that adds the server "name" for the message source
class	Server
{
	private:
		// First, the attributes
		int         			_socketFD;
		int         			_epollFD;
		sockaddr_in				_serverAddress;
		std::string				_password;
		std::queue<Message *>	_toProcess;
		std::map<int, std::string>	_partial_msgs;
		std::map<int, User*>	_clients;	// Maps socket FDs to Users
		std::map<std::string, Channel*>	_channels;
		const time_t			_creationTime;	// NOTE this is const because it is set once only

		// Some constructors that we want to disable / forbid
					Server(void);	// private so not called
					Server(const Server &irc);	// No good reason to allow copy construction of the server
		Server		operator=(const Server &irc);	// No assignment should be possible either

		// Unsorted mess of internal methods
		void		_printMessageQueue(std::queue<Message *> toPrint) const;
		void		_addNewClient();
		void		_removeUser(User &usr);
		bool		_isFullMsg(std::string msg) const;
		void		_storePartial(int fd_source, std::string msg);
		std::string	_getClientInput(int fd);

		// If this works, it is important
		void		_sendMessage(Message *to_send) const;
		void		_processQueue(void);	// runs through the Message queue and runs the Commands
		void		_cleanupServer();	// Limpieza de recursos del servidor

		// NOTE This final remaining "direct" send method is used in case of Errors
		// ...it may not be 100% necessary but is kept (for now) for safety
		void		_sendToFD(int fd, const std::string &text) const;

		ACommand*	_matchCmd(Message* do_next);

	public:
		Server(int port, std::string password);
		~Server(void);	// NOTE Destructor will have to cleanly close connections and whatever partial / pending messages we have

		void		run(void);
		bool		checkPasswd(std::string &to_check) const;

		// Simple public getters
		int			get_fd(void) const;
		std::string	getCreation(void) const;
		std::string	getUserModes(void) const;
		std::string	getChanModes(void) const;
		Channel*	findChannel(const std::string &name) const;
		User*		findUserByNick(const std::string &nick) const;
		std::map<std::string, Channel*>		getChannels(void) const;

		Channel*	createChannel(const std::string &name);
		// NOTE Must be public so that Part can remove empty channels
		void		removeChannel(const std::string &name);
		bool		isNickTaken(const std::string &nick, int except_fd = -1) const;

		// NOTE With restructuring this has to be public, may not be best
		void		sendWelcome(Message *msg);
		// NOTE So it can be called from QuitCmd
		void		handleError(Message *msg, User *usr);
};
#endif
