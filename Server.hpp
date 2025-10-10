#ifndef SERVER_HPP
# define SERVER_HPP

#include "ReplyEnums.hpp"	// for server numeric replies
#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>
#include <queue>// Messages to be processed
#include <set>	// FDs of clients to be sent to
#include <map>	// dictionary of partial messages
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
// [ ] PING
// [ ] PONG
// [ ] QUIT
// [ ] ERROR
// TODO We don't need both _clients and _moreClients, it looks stupid
// TODO A function that adds the server "name" for the message source
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
		std::map<int, User*>	_moreClients;	// TODO Potentially  this replaces _clients()
		std::map<std::string, Channel*>	_channels;
		time_t		_creationTime;	// TODO SHould this be const, static? It gets set once and never changes.

					Server(void);	// private so not called
					Server(const Server &irc);	// No good reason to allow copy construction of the server
		Server		operator=(const Server &irc);	// No assignment should be possible either

		void		_printMessageQueue(std::queue<Message *> toPrint) const;
		void		_addNewClient();
		void		_removeClient(struct epoll_event &bye);
		bool 		_setNonBlocking(int fd);
		bool		_isFullMsg(std::string msg) const;
		void		_storePartial(int fd_source, std::string msg);
		std::string	_getClientInput(int fd);
		bool		_isNickTaken(const std::string &nick, int except_fd = -1) const;
		// If this works, it is important
		void		_sendMessage(Message *to_send) const;

		// NOTE AS now, only works for Channel-related replies
		// TODO Should these "Message creation" functions be moved to that class?
		Message*	_replyNonNumeric(Message &msg, Channel *chan) const;
		Message*	_replyNonNumeric(Message &msg) const;

		// --- Helpers for channels/users ---
		User*		_findUserByNick(const std::string &nick) const;
		Channel*	_findChannel(const std::string &name) const;
		Channel*	_createChannel(const std::string &name);
		void		_removeChannel(const std::string &name);
		void		_sendToFD(int fd, const std::string &text) const;
		void		_broadcastToChannel(const std::string &chan, int from_fd, const std::string &text, bool include_sender=false) const;
		void		_broadcastToChannel(Channel *channel, int from_fd, const std::string &text, bool include_sender=false) const;

		// --- Manejo de comandos IRC ---
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
		void		handleAway(Message *msg, User *usr);

	public:
					Server(int port, std::string password);
					~Server(void);	// NOTE Destructor will have to cleanly close connections and whatever partial / pending messages we have

		int			get_fd(void) const;
		void		run(void);
//		bool		_checkPass(Message &msg) const;	//public to act as friend of Message
		void		_processQueue(void);	//public to act as friend of Message
		bool		normaliseChanName(std::string *chan);
		// HACK Public to be friend with message origin (user)
		Message*	_reply(Message &msg, int num_rep) const;
		Message*	_reply(Message &msg, int num_rep, Channel *chan) const;
		// NOTE Not sure if this one is any use
		Message*	_reply(Message &msg, int rep_code, User *usr) const;
		std::string	getUptime(void) const;
		std::string	getCreation(void) const;
};
#endif
