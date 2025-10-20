#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>
#include <queue>// Messages to be processed
#include <map>	// dictionary of partial messages
#include <list>	// parameters in user/channelMode
#include <ctime>

class	Message;
class	User;
class	Channel;

const std::string SERVERNAME	= "ft_irc";
const std::string VERSION	= "0.0.2";	// NOTE Increment this when we get bored

// What should this hold?
// - port
// - password (passed in on command line)
// TODO Commands to implement:
// [x] PING
// [x] PONG
// [x] QUIT
// [x] ERROR
// TODO A function that adds the server "name" for the message source
class	Server
{
	private:
		// First, the attributes
		int         _socketFD;
		int         _epollFD;
		sockaddr_in _serverAddress;
		std::string _password;
		std::queue<Message *>	_toProcess;
		std::map<int, std::string>	_partial_msgs;
		std::map<int, User*>	_clients;	// Maps socket FDs to Users
		std::map<std::string, Channel*>	_channels;
		time_t		_creationTime;	// TODO SHould this be const, static? It gets set once and never changes.

		// Some constructors that we want to disable / forbid
					Server(void);	// private so not called
					Server(const Server &irc);	// No good reason to allow copy construction of the server
		Server		operator=(const Server &irc);	// No assignment should be possible either

		// Unsorted mess of internal methods
		void		_printMessageQueue(std::queue<Message *> toPrint) const;
		void		_addNewClient();
		// FIXME Probably only need one of these two, or one should call the other
		void		_removeClient(struct epoll_event &bye);
		void		_removeUser(User &usr);
		bool 		_setNonBlocking(int fd);
		bool		_isFullMsg(std::string msg) const;
		void		_storePartial(int fd_source, std::string msg);
		std::string	_getClientInput(int fd);
		bool		_isNickTaken(const std::string &nick, int except_fd = -1) const;
		// If this works, it is important
		void		_sendMessage(Message *to_send) const;


		// --- Helpers for channels/users ---
		User*		_findUserByNick(const std::string &nick) const;
		Channel*	_findChannel(const std::string &name) const;
		Channel*	_createChannel(const std::string &name);
		void		_removeChannel(const std::string &name);
		void		_channelMode(Message *msg, User *usr, std::string target);
		void		_userMode(Message *msg, User *usr, std::string target);
		// TODO These three "direct"  comms methods should be removed, too dangerous
		void		_sendToFD(int fd, const std::string &text) const;
		void		_broadcastToChannel(const std::string &chan, int from_fd, const std::string &text, bool include_sender=false) const;
		void		_broadcastToChannel(Channel *channel, int from_fd, const std::string &text, bool include_sender=false) const;
		void		_sendWelcome(Message *msg, User *usr);

		// --- Manejo de comandos IRC ---
		// NOTE Possibly these could be abstracted away into a Command interface class?
		void        	handleKick(Message *msg, User *usr);
		void        	handlePrivmsg(Message *msg, User *usr);
		void		handlePass(Message *msg, User *usr);
		void		handleNick(Message *msg, User *usr);
		void		handleUser(Message *msg, User *usr);
		void		handleJoin(Message *msg, User *usr);
		void		handlePart(Message *msg, User *usr);
		void		handleNames(Message *msg, User *usr);
		void		handleList(Message *msg, User *usr);
		void		handleTopic(Message *msg, User *usr);
		void		handleInvite(Message *msg, User *usr);
		void		handleMode(Message *msg, User *usr);
		void		handlePing(Message *msg, User *usr);
		void		handleWho(Message *msg, User *usr);
		void		handleWhoIs(Message *msg, User *usr);
		void		handleAway(Message *msg, User *usr);
		void		handleQuit(Message *msg, User *usr);
		void		handleUserhost(Message *msg, User *usr);
		void		handleError(Message *msg, User *usr);

	public:
		Server(int port, std::string password);
		~Server(void);	// NOTE Destructor will have to cleanly close connections and whatever partial / pending messages we have

		// TODO Server::run() could be private, who else calls it?
		void		run(void);
//		bool		_checkPass(Message &msg) const;	//public to act as friend of Message
		void		_processQueue(void);	//public to act as friend of Message
		// TODO SHould this be a public method in Channel?
		bool		normaliseChanName(std::string *chan);
		// TODO All Message* returning functions might move to that class
		// Simple public getters
		int		get_fd(void) const;
		std::string	getUptime(void) const;
		std::string	getCreation(void) const;
		std::string	getUserModes(void) const;
		std::string	getChanModes(void) const;
};
#endif
