#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>
#include <queue>// Messages to be processed
#include <set>	// FDs of clients to be sent to
#include <map>	// dictionary of partial messages
#include <vector>

class	Message;
class	User;

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
		std::map<int, User*>	_moreClients;	// TODO Potentially  this replaces _clients()
		// --- Channels ---
		struct Channel {
			Channel(): name(), topic(), members(), operators(), invitedNicks(), topicProtected(false), inviteOnly(false) {}
			std::string			name;
			std::string			topic;
			std::set<int>		members;
			std::set<int>		operators; // fds with op rights
			std::set<std::string>	invitedNicks; // invite list by nick
			bool				topicProtected; // +t only ops can set topic
			bool				inviteOnly; // +i invite-only channel
		};
		std::map<std::string, Channel>	_channels;

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
		void		_reply(int send_to, int msg) const;
		bool		_isNickTaken(const std::string &nick, int except_fd = -1) const;

		// --- Helpers for channels/users ---
		User*		_findUserByNick(const std::string &nick) const;
		void		_sendToFD(int fd, const std::string &text) const;
		void		_broadcastToChannel(const std::string &chan, int from_fd, const std::string &text, bool include_sender=false) const;
		bool		_isChanOp(const Channel &c, int fd) const;
		void		_setChanOp(Channel &c, int fd, bool make_op);

		// --- Manejo de comandos IRC ---
		void        handleKick(Message *msg, int sender_fd);
		void        handlePrivmsg(Message *msg, int sender_fd);
		void		handleNick(Message *msg, User *usr);
		void		handleUser(Message *msg, User *usr);
		void		handleJoin(Message *msg, User *usr);
		void		handlePart(Message *msg, User *usr);
		void		handleNames(Message *msg, User *usr);
		void		handleList(Message *msg, User *usr);
		void		handleTopic(Message *msg, User *usr);
		void		handleInvite(Message *msg, User *usr);
		void		handleMode(Message *msg, User *usr);

	public:
					Server(int port, std::string password);
					~Server(void);	// NOTE Destructor will have to cleanly close connections and whatever partial / pending messages we have

		int			get_fd(void) const;
		void		run(void);
//		bool		_checkPass(Message &msg) const;	//public to act as friend of Message
		void		handlePass(Message *msg, User *usr) const;
		void		_processQueue(void);	//public to act as friend of Message
};
#endif
