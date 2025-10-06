#ifndef SERVER_HPP
# define SERVER_HPP

#include <sys/socket.h>	// socket() function
#include <netinet/in.h>	// provides the sockaddr_in struct
#include <string>
#include <queue>// Messages to be processed
#include <set>	// FDs of clients to be sent to
#include <map>	// dictionary of partial messages

class	Message;
class	User;
class	Channel;

// What should this hold?
// - port
// - password (passed in on command line)
// TODO Commands to implement:
// [ ] PING
// [ ] PONG
// [ ] QUIT
// [ ] ERROR
// TODO We don't need both _clients and _moreClients, it looks stupid
class	Server
{
	private:
		int         _socketFD;
		int         _epollFD;
		sockaddr_in _serverAddress;
		std::string _password;
		std::string _creationTime;
		std::queue<Message *>	_toProcess;
		std::set<int> _clients;		// NOTE This is the fds to be sent to; may duplicate other info
		std::map<int, std::string>	_partial_msgs;
		std::map<int, User*>	_moreClients;	// TODO Potentially  this replaces _clients()
		// --- Channels ---
		std::map<std::string, Channel*>	_channels;

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
		void		_reply(int send_to, int msg_code, const std::string &params = "") const;
		User*		_findUserByFD(int fd) const;
		std::string	_getChannelTopic(const std::string &channelName) const;
		std::string	_getChannelMembers(const std::string &channelName) const;
		size_t		_getChannelMemberCount(const std::string &channelName) const;
		std::string	_getLastInvitedUser() const;
		std::string	_getServerCreationDate() const;
		std::string	_getUserRealName(const std::string &nick) const;
		std::string	_getUserChannels(const std::string &nick) const;
		std::string	_getChannelModeString(const std::string &channelName) const;
		std::string	_getTopicSetter(const std::string &channelName) const;
		std::string	_getTopicTime(const std::string &channelName) const;
		bool		_isNickTaken(const std::string &nick, int except_fd = -1) const;

		// --- Helpers for channels/users ---
		User*		_findUserByNick(const std::string &nick) const;
		Channel*	_findChannel(const std::string &name) const;
		Channel*	_createChannel(const std::string &name);
		void		_removeChannel(const std::string &name);
		void		_sendToFD(int fd, const std::string &text) const;
		void		_broadcastToChannel(const std::string &chan, int from_fd, const std::string &text, bool include_sender=false) const;
		void		_broadcastToChannel(Channel *channel, int from_fd, const std::string &text, bool include_sender=false) const;

		// --- Manejo de comandos IRC ---
		void        handleKick(Message *msg, User *usr);
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
		void		handlePass(Message *msg, User *usr);
		void		_processQueue(void);	//public to act as friend of Message
		bool		normaliseChanName(std::string *chan);
		void		sendWelcomeMessages(User *user);
};
#endif
