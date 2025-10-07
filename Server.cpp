#include "Server.hpp"
#include "Message.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include "IrcConstants.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>	// for close()
#include <sstream>
#include <sys/epoll.h>
#include <cerrno>	// for error checking in send calls
// #include <type_traits>	// NO this is C++11 feature :(
#include <cstdlib>	// for the EXIT code
#include <cstring>	// for memset. Too many includes!
#include <fcntl.h>	// NOTE IS there a C++ equivalent we should prefer?
					// RESPONSE No, fcntl is what we need to use because we cant use external libraries and stl has nothing better or equal.
					// ***
					// Subject IV.2 *** For MacOS only ***
					// ***
					// Since MacOS does not implement write() in the same
					// way as other Unix OSes, you are permitted to use fcntl().
					// However, you are allowed to use fcntl() only as follows:
					// fcntl(fd, F_SETFL, O_NONBLOCK);
					// Any other flag is forbidden.
					//
					// ...we maybe are supposed to directly create a non-blocking socket,
					// https://stackoverflow.com/a/63348937


// Helper para poner un socket en modo no bloqueante
// Get the existing flags for the newly-accepted clientSocket
// Add non-blocking to those existing client flags
// TODO Check everywhere for fcntl calls which should be replaced by this
bool Server::_setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return (false);
	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

// _clients debe ser miembro de la clase Server, no global
// Set up the Server:
// - create fd for socket
// - set as non-blocking
// - set to listen on IP4, user-supplied port and any incoming IP
// - bind socket to the fd
// - listen on fd
// - Create epoll fd
// - Add socket fd to epoll's listening set
// FIXED There are no try/catch blocks for the thrown exceptions
Server::Server(int port, std::string password) : _socketFD(0), _epollFD(0),
												 _serverAddress(), _password(password),
												 _creationTime(), _toProcess(), _clients(), _partial_msgs(),
												 _moreClients(), _channels()
{
	std::cout << "Server constructor with parameters called" << std::endl;
	
	// Set creation time
	time_t now = time(0);
	struct tm *timeinfo = localtime(&now);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a %b %d %Y at %H:%M:%S %Z", timeinfo);
	_creationTime = std::string(buffer);
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFD == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	std::cout << "Created a socket listening at fd " << _socketFD << std::endl;

	// Set socket to non-blocking by:
	// Get the existing flags for the newly-created Socket
	// FIXME use helper function?
	int flags = fcntl(_socketFD, F_GETFL, 0);
	// Add non-blocking to those existing client flags
	fcntl(_socketFD, F_SETFL, flags | O_NONBLOCK);

	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	std::cout << "Binding...";
	// Proper cleanup on binding failure
	if (bind(_socketFD, (struct sockaddr *)&_serverAddress, sizeof(_serverAddress)) == -1) 
	{
		close(_socketFD);
		throw std::runtime_error("Binding failed");
	}
	std::cout << " Socket successfully bound" << std::endl;

	std::cout << "Listening..." << std::endl;
	if (listen(_socketFD, 5) == -1) 
	{
		close(_socketFD);
		throw std::runtime_error("Listening set up failed");
	}
	std::cout << "Server ready to accept connections." << std::endl;

	// Crear epoll
	_epollFD = epoll_create1(0);
	if (_epollFD == -1)
	{
		close(_socketFD);
		throw std::runtime_error("Could not create epoll");
	}

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET; // Edge-triggered for better performance
	ev.data.fd = _socketFD;
	if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &ev) == -1) 
	{
		close(_socketFD);
		close(_epollFD);
		throw std::runtime_error("Could not add server input to epoll set.");
	}
}

// What needs to be done when we get the first contact from a new client
// - Accept
// - set the socket as non-blocking
// - make an event struct for its input
// - add that to the epollFD that we monitor.
// NOTE If something goes wrong, throws a runtime_error exception to be caught outside
// TODO Make proper use of the newUser! (WHAT DOES THAT MEAN?)
void	Server::_addNewClient()
{
	try
	{
		int clientSocket = accept(this->_socketFD, NULL, NULL);

		if (clientSocket == -1)
			throw std::runtime_error("Could not get client socket");
		std::cout << "Nuevo cliente conectado, fd: " << clientSocket << std::endl;
		// Hacer el socket del cliente no bloqueante
		if (!_setNonBlocking(clientSocket))
		{
			close (clientSocket);
			throw std::runtime_error("Failed to set client socket non-blocking!");
		}
		User	*newUser = User::makeUser(clientSocket);
		std::cout << "Created a new user from fd" << clientSocket << std::endl;
		this->_moreClients[clientSocket] = newUser;
		// Añadir el cliente a epoll
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = clientSocket;
		//if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, clientSocket, &ev) == -1)
		if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, newUser->getFD(), &ev) == -1)
		{
			close(clientSocket);
			throw std::runtime_error("Could not add client socket to epoll");
		}
		else
		{
			this->_clients.insert(newUser->getFD());
		}
	}
	catch (std::exception &e)
	{
		std::cerr << "User creation failure: " << e.what() << std::endl;
		// Continue server operation instead of crashing
	}
}

// Small wrapper for dealing with a client that has disconnected.
// - close its fd
// - remove that fd from the epoll listening set
// NOTE Should the events array we pass in be stored as part of the class instead?
// NOTE This could work on an int fd only, but this allows other actions if needed.
// DONE Once we have User instances, this should remove those too
// TODO All of the Channels that contain this User should be notified of the disconnection
void	Server::_removeClient(struct epoll_event &goodbye)
{
	std::cout << "Cliente desconectado, fd: " << goodbye.data.fd << std::endl;
	
	// Find user and notify all channels they're leaving
	std::map<int, User*>::iterator it = this->_moreClients.find(goodbye.data.fd);
	if (it != this->_moreClients.end())
	{
		User *user = it->second;
		if (user)
		{
			// Notify all channels the user is on
			for (std::map<std::string, Channel*>::iterator chan_it = _channels.begin(); chan_it != _channels.end(); ++chan_it)
			{
				Channel *channel = chan_it->second;
				if (channel->isMember(goodbye.data.fd))
				{
					channel->removeMember(goodbye.data.fd);
					// If channel is empty, remove it
					if (channel->isEmpty())
					{
						delete channel;
						_channels.erase(chan_it);
						// Reset iterator to avoid invalidation
						chan_it = _channels.begin();
						if (chan_it == _channels.end())
							break;
					}
				}
			}
		}
		delete it->second;
		this->_moreClients.erase(it);
	}
	
	// Clean up resources
	close(goodbye.data.fd);
	epoll_ctl(_epollFD, EPOLL_CTL_DEL, goodbye.data.fd, NULL);
	_clients.erase(goodbye.data.fd);
	this->_partial_msgs.erase(goodbye.data.fd);
}

// Activates the Server's epoll loop
// - Waits for an event
// -- if the event is on the Server fd, we treat it as a new connection
// --- i.e. we add it to the set of Things Listened For
// --- first setting it as nonblocking
// -- if it is *not* on the server fd then it is from a client
// TODO The errors (anything on std::cerr) should throw exception of some kind
// TODO Handle client disconnnections better: currently cause "failed to read new input"
// TODO MAX_EVENTS is better as a class variable
// TODO refactor out the "message to queue" part of the loop, it's too long
void Server::run()
{
	std::cout << "server on, waiting for conections (epoll)..." << std::endl;
	const int MAX_EVENTS = 10;
	struct epoll_event events[MAX_EVENTS];
	while (true)
	{
		// n is the number of fds ready for action
		int n = epoll_wait(_epollFD, events, MAX_EVENTS, -1);
		if (n == -1)
		{
			if (errno == EINTR)
			{
				// Interrupted by signal, continue
				continue;
			}
			std::cerr << "epoll_wait error: " << strerror(errno) << std::endl;
			// Don't break, try to continue server operation
			continue;
		}
		for (int i = 0; i < n; ++i)
		{
			// Input on Server, i.e. a new connection
			if (events[i].data.fd == _socketFD)
			{
				try
				{
					_addNewClient();
				}
				catch (std::exception &e)
				{
					std::cerr << "Failed to add new client: " << e.what() << std::endl;
					// Try to recover by cleaning up any partial state
					// The server continues running instead of crashing
				}
			}
			else	// Input from a client, written to a Message or stored
			{
				try
				{
					std::string	str_buf = _getClientInput(events[i].data.fd);
					if (!this->_isFullMsg(str_buf))	// buffer does not form a complete message
					{
						std::cout << "Can NOT be parsed, store partial" << std::endl;
						this->_storePartial(events[i].data.fd, str_buf);
//						std::cout << "Done. Stored:" << _partial_msgs[events[i].data.fd] << std::endl;
					}
					else	// Parse into Message and queue for further action
					{
						// With a complete message, must delete partials
						this->_partial_msgs[events[i].data.fd].erase();
						std::cout << "Can be parsed" << std::endl;
						User*	msgFrom =  this->_moreClients[events[i].data.fd];
						Message	*nxtMessage = Message::makeMessage(str_buf, msgFrom);
						this->_toProcess.push(nxtMessage);
					}
					std::cout << "Mensaje recibido de fd " << events[i].data.fd << ": " << str_buf << std::endl;
					// NOTE: No global broadcast here. Message dispatch happens via parsed commands (e.g., PRIVMSG)
					// Message processed successfully
				}
				// TODO Work out how to handle / merge the 2 different exceptions.
				catch (std::exception &e)
				{
					std::cerr << "Error processing client " << events[i].data.fd << ": " << e.what() << std::endl;
					// Don't disconnect client for command errors, just log and continue
					// Only disconnect for serious connection issues
					if (std::string(e.what()).find("disconnected") != std::string::npos ||
						std::string(e.what()).find("Failed to read") != std::string::npos)
					{
						_removeClient(events[i]);
					}
					// For other errors (like invalid commands), just continue
				}
				// catch (std::exception &e)
				// {
				// 	std::cerr << "Something wrong with message: ignore and continue." << std::endl;
				// 	std::cerr << e.what() <<std::endl;
				// }
			}
		}
		_processQueue();
	}
}

// Simple gettter for the Server socket's file descriptor.
int	Server::get_fd(void) const
{
	return (this->_socketFD);
}

// Debug function to view a message queue.
// Uses a copy not a reference so that we don't lose item
// (Yes that is probably *very* inefficient)
void	Server::_printMessageQueue(std::queue<Message *> toPrint) const
{
	Message	*this_one;
	int	n;
	int	i = 1;

	n = toPrint.size();
	std::cout << "Printing message queue with " << n << " items" << std::endl;
	while (toPrint.empty() != true)
	{
		this_one = toPrint.front();
		std::cout << "MSG: " << i++ <<std::endl << *this_one;
		toPrint.pop();
	}
	std::cout << n << " Messages printed" << std::endl;
}

// Manejo de comandos IRC ---
// Esqueleto para el comando KICK
// TODO Remove repetitive parts like checking the channel name, that should be in Channel::_findChannel
// TODO isOperator() probably is based on NICK or USER not a fd? What happens if they reconnect?
// TODO Wrong number of parameters needs an error message sent
// TODO Unified message creation / sending not the hardcoded parameters
void Server::handleKick(Message *msg, User *usr)
{
	std::list<std::string> params = msg->getParams();
	if (params.size() < 2)
	{
		this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "KICK");
		return ;
	}
	
	std::string chan = params.front();
	params.pop_front();
	std::string nick = params.front();
	params.pop_front();
	
	// Get reason if provided
	std::string reason = "";
	if (!params.empty())
	{
		reason = params.front();
		if (!reason.empty() && reason[0] == ':')
			reason.erase(0, 1);
	}
	if (reason.empty())
		reason = usr->getNick(); // Default reason is kicker's nick
	
	// Normalize channel name
	if (!this->normaliseChanName(&chan))
	{
		this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
		return ;
	}
	
	Channel *channel = _findChannel(chan);
	if (!channel)
	{
		this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
		return ;
	}
	
	// Check if sender is on channel
	if (!channel->isMember(usr->getFD()))
	{
		this->_reply(usr->getFD(), ERR_NOTONCHANNEL, chan);
		return ;
	}
	
	// Sender must be channel operator
	if (!channel->isOperator(usr->getFD()))
	{
		this->_reply(usr->getFD(), ERR_CHANOPRIVSNEEDED, chan);
		return ;
	}
	
	// Find target user
	User *target = _findUserByNick(nick);
	if (!target)
	{
		this->_reply(usr->getFD(), ERR_NOSUCHNICK, nick);
		return ;
	}
	
	// Target must be member of channel
	if (!channel->isMember(target->getFD()))
	{
		this->_reply(usr->getFD(), ERR_USERNOTINCHANNEL, nick + " " + chan);
		return ;
	}
	
	// Cannot kick yourself
	if (target->getFD() == usr->getFD())
	{
		this->_reply(usr->getFD(), ERR_USERNOTINCHANNEL, nick + " " + chan);
		return ;
	}
	
	// Remove target from channel
	channel->removeMember(target->getFD());
	target->removeChannel(chan);
	
	// Send KICK message to channel and target
	std::string kickMsg = ":" + usr->getNick() + " KICK " + chan + " " + nick + " :" + reason;
	_broadcastToChannel(channel, -1, kickMsg, true);
	_sendToFD(target->getFD(), kickMsg + MSG_TERMINATOR);
}

// TODO use Server_reply for the 404
// TODO There are further checks needed on whether a message is allowed, see docs
// Sends a message to user(s) or channel(s)
// https://modern.ircdocs.horse/#privmsg-message
void Server::handlePrivmsg(Message *msg, int sender_fd)
{
	User *sender = _findUserByFD(sender_fd);
	if (!sender)
		return ;
		
	std::list<std::string> params = msg->getParams();
	if (params.empty())
	{
		this->_reply(sender_fd, ERR_NORECIPIENT, "PRIVMSG");
		return ;
	}
	
	if (params.size() < 2)
	{
		this->_reply(sender_fd, ERR_NOTEXTTOSEND);
		return ;
	}
	
	std::string target = params.front();
	params.pop_front();
	std::string text = params.front();
	
	if (text.empty())
	{
		this->_reply(sender_fd, ERR_NOTEXTTOSEND);
		return ;
	}
	
	// Channel message
	if (!target.empty() && target[0] == '#')
	{
		Channel *channel = _findChannel(target);
		if (!channel)
		{
			this->_reply(sender_fd, ERR_NOSUCHCHANNEL, target);
			return ;
		}
		if (!channel->isMember(sender_fd))
		{
			this->_reply(sender_fd, ERR_CANNOTSENDTOCHAN, target);
			return ;
		}
		
		// Send PRIVMSG to channel
		std::string privmsg = ":" + sender->getNick() + " PRIVMSG " + target + " :" + text;
		_broadcastToChannel(channel, sender_fd, privmsg, true);
	}
	else
	{
		// Private message to user
		User *to = _findUserByNick(target);
		if (!to)
		{
			this->_reply(sender_fd, ERR_NOSUCHNICK, target);
			return ;
		}
		
		// Send PRIVMSG to user
		std::string privmsg = ":" + sender->getNick() + " PRIVMSG " + target + " :" + text;
		_sendToFD(to->getFD(), privmsg + MSG_TERMINATOR);
	}
}

// Get user
// Get parameters
// Check the requested nick is valid and does not already exist
// Set new nickname on User (will need a setter on User?)
void	Server::handleNick(Message *msg, User *usr)
{
	std::list<std::string>	_params = msg->getParams();
	if (_params.empty())
	{
		this->_reply(usr->getFD(), ERR_NONICKNAMEGIVEN);
		return ;
	}
	std::string	newNick = _params.front();
	
	// Security: Validate nickname length
	if (newNick.length() > 9 || newNick.length() == 0)
	{
		this->_reply(usr->getFD(), ERR_ERRONEUSNICKNAME, newNick);
		return ;
	}
	
	// Security: Check for forbidden characters
	std::string	notLeading = "#:&@123456789";
	std::string forbidden = " \b\n\r\0";
	
	// Security: Additional validation for special characters
	for (size_t i = 0; i < newNick.length(); ++i)
	{
		char c = newNick[i];
		if (c < 32 || c > 126 || c == ',' || c == ':' || c == '\\' || c == '\'' || c == '"')
		{
			this->_reply(usr->getFD(), ERR_ERRONEUSNICKNAME, newNick);
			return ;
		}
	}

	if ((newNick.find_first_of(notLeading) == 0) ||
		(newNick.find_first_of(forbidden) != std::string::npos))
	{
		this->_reply(usr->getFD(), ERR_ERRONEUSNICKNAME, newNick);
		return;
	}
	else if (_isNickTaken(newNick, usr->getFD()))
	{
		this->_reply(usr->getFD(), ERR_NICKNAMEINUSE, newNick);
		return;
	}
	else
	{
		usr->setNick(newNick);
		
		// Check if user is now fully registered and send welcome messages
		if (usr->isRegistered())
		{
			sendWelcomeMessages(usr);
		}
	}
}

// FIXED Protect against empty _params!
// TODO 3 parameters is probably OK, be less strict and fill gaps with NICK
void	Server::handleUser(Message *msg, User *usr)
{
	std::list<std::string>	_params = msg->getParams();
	if (_params.size() != 4)
	{
		this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "USER");
		return ;
	}
	
	// Validate username parameter
	std::string username = _params.front();
	if (username.empty() || username.length() > 9)
	{
		this->_reply(usr->getFD(), ERR_ERRONEUSNICKNAME, username);
		return ;
	}
	std::string	newUser = _params.front();
	// Skip to the final entry (used the first, ingore the middle 2)
	_params.pop_front();
	_params.pop_front();
	_params.pop_front();
	std::string	newRName = _params.front();
	if (newUser.empty())
		newUser = usr->getNick();
	if (newRName.empty())
		newRName = usr->getReal();
	usr->setUser(newUser);
	usr->setReal(newRName);
	std::cout << "User: " << newUser << ", Really: " << newRName << std::endl;
	
	// Check if user is now fully registered and send welcome messages
	if (usr->isRegistered())
	{
		sendWelcomeMessages(usr);
	}
}

// FIXME This does not cause clients to realise they have joined a room :|
// TODO Send acknowledgements per https://modern.ircdocs.horse/#join-message
// [ ] A JOIN message with the client as the message <source> and the channel they have joined as the first parameter of the message.
// [ ] The channel’s topic (with RPL_TOPIC (332) and optionally RPL_TOPICWHOTIME (333)), and no message if the channel does not have a topic.
// [ ] A list of users currently joined to the channel (with one or more RPL_NAMREPLY (353) numerics followed by a single RPL_ENDOFNAMES (366) numeric). These RPL_NAMREPLY messages sent by the server MUST include the requesting client that has just joined the channel.
// TODO Break out the name normalisation to a helper function
// TODO JOIN can accept an alternative parameter of '0'
// TODO Improve parameter handling so JOIN Can handle multiple Channels
void	Server::handleJoin(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "JOIN");
        return ;
    }
    
    std::string chan = params.front();
    // Normalize channel name
    if (!this->normaliseChanName(&chan))
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }

    Channel *channel = this->_findChannel(chan);
    if (!channel)
        channel = this->_createChannel(chan);
    
    // Check if user is already on channel
    if (channel->isMember(usr->getFD()))
    {
        return ; // Already on channel
    }
    
    // Check if channel is full
    if (channel->getUserLimit() > 0 && channel->getMemberCount() >= (size_t)channel->getUserLimit())
    {
        this->_reply(usr->getFD(), ERR_CHANNELISFULL, chan);
        return ;
    }
    
    // Check if user is banned
    if (channel->isBanned(usr->getNick()))
    {
        this->_reply(usr->getFD(), ERR_BANNEDFROMCHAN, chan);
        return ;
    }
    
    // Check if channel has password
    if (channel->hasPassword())
    {
        if (params.size() < 2)
        {
            this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "JOIN");
            return ;
        }
        std::string password = params.back();
        if (password != channel->getPassword())
        {
            this->_reply(usr->getFD(), ERR_BADCHANNELKEY, chan);
            return ;
        }
    }
    
    // Check if channel is invite only
    if (channel->isInviteOnly())
    {
        if (!channel->isInvited(usr->getNick()))
        {
            this->_reply(usr->getFD(), ERR_INVITEONLYCHAN, chan);
            return ;
        }
        else
        {
            channel->removeInvite(usr->getNick());
        }
    }
    
    // Add user to channel
    if (channel->addMember(usr->getFD()))
    {
        usr->addChannel(chan);
        
        // Send JOIN message to all channel members
        std::string join_msg = ":" + usr->getNick() + " JOIN :" + chan;
        _broadcastToChannel(channel, usr->getFD(), join_msg, true);
        
        // Send topic if exists
        if (!channel->getTopic().empty())
        {
            this->_reply(usr->getFD(), RPL_TOPIC, chan);
        }
        else
        {
            this->_reply(usr->getFD(), RPL_NOTOPIC, chan);
        }
        
        // Send names list
        this->_reply(usr->getFD(), RPL_NAMREPLY, chan);
        this->_reply(usr->getFD(), RPL_ENDOFNAMES, chan);
    }
}

// TODO Factor out the channel name normalisation, it is repeated everywhere
// TODO Unsuccessful commands will need an Error reply
// TODO Use a standard function to craft message, not hardcoded parameters
void	Server::handlePart(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "PART");
        return ;
    }
    
    std::string chan = params.front();
    // Normalize channel name
    if (!this->normaliseChanName(&chan))
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }
    
    Channel *channel = _findChannel(chan);
    if (!channel)
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }
    
    // Check if user is on channel
    if (!channel->isMember(usr->getFD()))
    {
        this->_reply(usr->getFD(), ERR_NOTONCHANNEL, chan);
        return ;
    }
    
    // Get reason if provided
    std::string reason = "";
    if (params.size() > 1)
    {
        reason = params.back();
        if (!reason.empty() && reason[0] == ':')
            reason.erase(0, 1);
    }
    
    if (channel->removeMember(usr->getFD()))
    {
        usr->removeChannel(chan);
        
        // Send PART message to all channel members
        std::string part_msg = ":" + usr->getNick() + " PART " + chan;
        if (!reason.empty())
            part_msg += " :" + reason;
        _broadcastToChannel(channel, -1, part_msg, true);
        
        // If channel is empty, remove it
        if (channel->isEmpty())
            _removeChannel(chan);
    }
}

// FIXME Does not comply with specifications of NAMES command
// https://modern.ircdocs.horse/#names-message
// TODO Read msg parameters and call to each named channel
void	Server::handleNames(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    
    if (params.empty())
    {
        // List all visible channels
        for (std::map<std::string, Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
        {
            const std::string &chan = it->first;
            this->_reply(usr->getFD(), RPL_NAMREPLY, chan);
        }
    }
    else
    {
        // List specific channels
        for (std::list<std::string>::const_iterator it = params.begin(); it != params.end(); ++it)
        {
            std::string chan = *it;
            if (this->normaliseChanName(&chan))
            {
                Channel *channel = _findChannel(chan);
                if (channel)
                {
                    this->_reply(usr->getFD(), RPL_NAMREPLY, chan);
                }
            }
        }
    }
    
    // Send end of names
    this->_reply(usr->getFD(), RPL_ENDOFNAMES, "");
}

// TODO Check this against specification: https://modern.ircdocs.horse/#list-message
// NOTE Optionally takes parameters, should not ignore msg
void	Server::handleList(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    
    // Send list start
    this->_reply(usr->getFD(), RPL_LISTSTART);
    
    if (params.empty())
    {
        // List all channels
        for (std::map<std::string, Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
        {
            Channel *c = it->second;
            this->_reply(usr->getFD(), RPL_LIST, c->getName());
        }
    }
    else
    {
        // List specific channels
        for (std::list<std::string>::const_iterator it = params.begin(); it != params.end(); ++it)
        {
            std::string chan = *it;
            if (this->normaliseChanName(&chan))
            {
                Channel *channel = _findChannel(chan);
                if (channel)
                {
                    this->_reply(usr->getFD(), RPL_LIST, channel->getName());
                }
            }
        }
    }
    
    // Send end of list
    this->_reply(usr->getFD(), RPL_LISTEND);
}

void	Server::handleTopic(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "TOPIC");
        return ;
    }
    
    std::string chan = params.front();
    // Normalize channel name
    if (!this->normaliseChanName(&chan))
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }
    
    Channel *channel = _findChannel(chan);
    if (!channel)
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }
    
    // Check if user is on channel
    if (!channel->isMember(usr->getFD()))
    {
        this->_reply(usr->getFD(), ERR_NOTONCHANNEL, chan);
        return ;
    }
    
    if (params.size() == 1)
    {
        // Send current topic
        if (channel->getTopic().empty()) {
            this->_reply(usr->getFD(), RPL_NOTOPIC, chan);
        } else {
            this->_reply(usr->getFD(), RPL_TOPIC, chan);
        }
        return ;
    }
    if (channel->isTopicProtected() && !channel->isOperator(usr->getFD()))
    {
        this->_reply(usr->getFD(), ERR_CHANOPRIVSNEEDED, chan);
        return ;
    }
    params.pop_front();
    std::string newTopic = params.front();
    if (!newTopic.empty() && newTopic[0] == ':')
        newTopic.erase(0, 1);
    channel->setTopic(newTopic);
    
    // Send TOPIC message to all channel members
    std::string topicMsg = ":" + usr->getNick() + " TOPIC " + chan + " :" + newTopic;
    _broadcastToChannel(channel, -1, topicMsg, true);
}

void	Server::handleInvite(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.size() < 2)
    {
        this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "INVITE");
        return ;
    }
    
    std::string nick = params.front();
    params.pop_front();
    std::string chan = params.front();
    
    // Normalize channel name
    if (!this->normaliseChanName(&chan))
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }
    
    Channel *channel = _findChannel(chan);
    if (!channel)
    {
        this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
        return ;
    }
    
    // Check if user is on channel
    if (!channel->isMember(usr->getFD()))
    {
        this->_reply(usr->getFD(), ERR_NOTONCHANNEL, chan);
        return ;
    }
    
    // Check if user is channel operator
    if (!channel->isOperator(usr->getFD()))
    {
        this->_reply(usr->getFD(), ERR_CHANOPRIVSNEEDED, chan);
        return ;
    }
    
    // Find target user
    User *target = _findUserByNick(nick);
    if (!target)
    {
        this->_reply(usr->getFD(), ERR_NOSUCHNICK, nick);
        return ;
    }
    
    // Check if target is already on channel
    if (channel->isMember(target->getFD()))
    {
        this->_reply(usr->getFD(), ERR_USERONCHANNEL, nick + " " + chan);
        return ;
    }
    
    // Add invite and send messages
    channel->addInvite(nick);
    
    // Send RPL_INVITING to inviter
    this->_reply(usr->getFD(), RPL_INVITING, nick + " " + chan);
    
    // Send INVITE message to target
    std::string inviteMsg = ":" + usr->getNick() + " INVITE " + nick + " " + chan;
    _sendToFD(target->getFD(), inviteMsg + MSG_TERMINATOR);
}

void	Server::handleMode(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "MODE");
        return ;
    }
    
    std::string target = params.front();
    params.pop_front();
    
    // Channel mode
    if (!target.empty() && target[0] == '#')
    {
        std::string chan = target;
        // Normalize channel name
        if (!this->normaliseChanName(&chan))
        {
            this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
            return ;
        }
        
        Channel *channel = _findChannel(chan);
        if (!channel)
        {
            this->_reply(usr->getFD(), ERR_NOSUCHCHANNEL, chan);
            return ;
        }
        
        // Check if user is on channel
        if (!channel->isMember(usr->getFD()))
        {
            this->_reply(usr->getFD(), ERR_NOTONCHANNEL, chan);
            return ;
        }
        
        // If no mode flags provided, show current modes
        if (params.empty())
        {
            this->_reply(usr->getFD(), RPL_CHANNELMODEIS, chan);
            return ;
        }
        
        // Check if user is channel operator
        if (!channel->isOperator(usr->getFD()))
        {
            this->_reply(usr->getFD(), ERR_CHANOPRIVSNEEDED, chan);
            return ;
        }
        
        std::string flags = params.front();
        params.pop_front();
        
        bool adding = true;
        std::string modeStr = "";
        bool modeChanged = false;
        
        for (size_t i = 0; i < flags.size(); ++i)
        {
            char f = flags[i];
            if (f == '+') { adding = true; continue; }
            if (f == '-') { adding = false; continue; }
            
            std::string param = "";
            if (!params.empty())
            {
                param = params.front();
                params.pop_front();
            }
            
            if (channel->setMode(f, adding, param))
            {
                modeStr += (adding ? "+" : "-") + std::string(1, f);
                if (!param.empty())
                    modeStr += " " + param;
                modeChanged = true;
            }
            else
            {
                // Mode not supported or invalid
                this->_reply(usr->getFD(), ERR_UNKNOWNMODE, std::string(1, f));
                return ;
            }
        }
        
        // Broadcast mode change to channel
        if (modeChanged)
        {
            std::string modeMsg = ":" + usr->getNick() + " MODE " + chan + " " + modeStr;
            _broadcastToChannel(channel, -1, modeMsg, true);
        }
    }
    else
    {
        // User mode (not implemented for ft_irc)
        this->_reply(usr->getFD(), ERR_UMODEUNKNOWNFLAG);
    }
}

void	Server::handlePing(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_reply(usr->getFD(), ERR_NOORIGIN);
        return ;
    }
    
    std::string origin = params.front();
    if (origin.empty())
    {
        this->_reply(usr->getFD(), ERR_NOORIGIN);
        return ;
    }
    
    // Send PONG response
    std::string pongMsg = ":" + std::string(MSG_PREFIX_SERVER).substr(1) + " PONG " + std::string(MSG_PREFIX_SERVER).substr(1) + " :" + origin;
    _sendToFD(usr->getFD(), pongMsg + MSG_TERMINATOR);
}

void	Server::handlePong(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_reply(usr->getFD(), ERR_NOORIGIN);
        return ;
    }
    
    // PONG is typically just acknowledged, no specific response needed
    // The server can use this to verify the client is still alive
    (void)params; // Suppress unused parameter warning
    (void)usr;    // Suppress unused parameter warning
}

void	Server::handleQuit(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    std::string reason = "";
    
    if (!params.empty())
    {
        reason = params.front();
        if (!reason.empty() && reason[0] == ':')
            reason.erase(0, 1);
    }
    
    if (reason.empty())
        reason = "Quit: " + usr->getNick();
    else
        reason = "Quit: " + reason;
    
    // Send QUIT message to all channels the user is on
    std::string quitMsg = ":" + usr->getNick() + " QUIT :" + reason;
    
    // Get all channels the user is on and broadcast QUIT message
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        Channel *channel = it->second;
        if (channel->isMember(usr->getFD()))
        {
            _broadcastToChannel(channel, usr->getFD(), quitMsg, false);
            channel->removeMember(usr->getFD());
        }
    }
    
    // Remove user from server
    _clients.erase(usr->getFD());
    _partial_msgs.erase(usr->getFD());
    _moreClients.erase(usr->getFD());
    
    // Close the connection
    close(usr->getFD());
    epoll_ctl(_epollFD, EPOLL_CTL_DEL, usr->getFD(), NULL);
    
    // Delete user object
    delete usr;
}

void	Server::handleError(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    std::string errorMsg = "";
    
    if (!params.empty())
    {
        errorMsg = params.front();
        if (!errorMsg.empty() && errorMsg[0] == ':')
            errorMsg.erase(0, 1);
    }
    
    if (errorMsg.empty())
        errorMsg = "Connection closed by client";
    
    // Log the error
    std::cerr << "ERROR from " << usr->getNick() << ": " << errorMsg << std::endl;
    
    // Remove user from all channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        Channel *channel = it->second;
        if (channel->isMember(usr->getFD()))
        {
            channel->removeMember(usr->getFD());
        }
    }
    
    // Remove user from server
    _clients.erase(usr->getFD());
    _partial_msgs.erase(usr->getFD());
    _moreClients.erase(usr->getFD());
    
    // Close the connection
    close(usr->getFD());
    epoll_ctl(_epollFD, EPOLL_CTL_DEL, usr->getFD(), NULL);
    
    // Delete user object
    delete usr;
}

Server::~Server(void)
{
	// Clean up all resources properly
	std::cout << "Server destructor called." << std::endl;
	
	// Close all client connections
	for (std::map<int, User*>::iterator it = _moreClients.begin(); it != _moreClients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	_moreClients.clear();
	
	// Clean up channels
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		delete it->second;
	}
	_channels.clear();
	
	// Close server socket and epoll
	if (_socketFD > 0)
		close(_socketFD);
	if (_epollFD > 0)
		close(_epollFD);
}

// TODO Consider being more lenient and allowing \r only to terminate commands
bool	Server::_isFullMsg(std::string msg) const
{
	size_t	pos;

	pos = msg.find('\n');
	if (pos == std::string::npos)
		return (false);
	else
		return (true);
}

// TODO This should first check for existing text there!
void	Server::_storePartial(int fd_source, std::string msg)
{
	this->_partial_msgs[fd_source] = msg;
}

// return a string that can be used as a possible Message
// Read the input fd and combine with any existing partial data
std::string	Server::_getClientInput(int fd)
{
	char		buf[512];
	std::string	ret_val;
	int			chars_already = 0;
	int			new_chars = 0;

	memset(buf, '\0', 512);
	if (this->_partial_msgs.count(fd) != 0)
	{
		ret_val = this->_partial_msgs[fd];
		chars_already = ret_val.length();
	}
	
	// Security: Check if message is already too long
	if (chars_already >= MSG_LEN)
	{
		throw std::runtime_error("Message too long");
	}
	
	new_chars = read(fd, buf, (sizeof(buf) - 1 - chars_already));
	if (new_chars < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// No data available right now, return what we have
			return (ret_val);
		}
		throw std::runtime_error("Failed to read input: " + std::string(strerror(errno)));
	}
	if (new_chars == 0)
	{
		// Client disconnected
		throw std::runtime_error("Client disconnected");
	}
	
	ret_val.append(buf);
	
	// Security: Check total message length doesn't exceed IRC limit
	if (ret_val.length() > MSG_LEN)
	{
		throw std::runtime_error("Message exceeds maximum length");
	}
	
	return (ret_val);
}

User* Server::_findUserByNick(const std::string &nick) const
{
    for (std::map<int, User*>::const_iterator it = this->_moreClients.begin(); it != this->_moreClients.end(); ++it)
    {
        if (it->second && it->second->getNick() == nick)
            return it->second;
    }
    return 0;
}

Channel* Server::_findChannel(const std::string &name) const
{
    std::map<std::string, Channel*>::const_iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    return 0;
}

Channel* Server::_createChannel(const std::string &name)
{
    Channel *channel = new Channel(name);
    _channels[name] = channel;
    return channel;
}

void Server::_removeChannel(const std::string &name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
    {
        delete it->second;
        _channels.erase(it);
    }
}

void	Server::_sendToFD(int fd, const std::string &text) const
{
    ssize_t bytes_written = write(fd, text.c_str(), text.size());
    if (bytes_written < 0)
    {
        if (errno == EPIPE || errno == ECONNRESET)
        {
            // Client disconnected, this is handled by epoll
            return;
        }
        std::cerr << "Error sending message to fd " << fd << ": " << strerror(errno) << std::endl;
    }
    else if (bytes_written < static_cast<ssize_t>(text.size()))
    {
        std::cerr << "Warning: Partial write to fd " << fd << " (" << bytes_written << "/" << text.size() << " bytes)" << std::endl;
    }
}

void	Server::_broadcastToChannel(const std::string &chan, int from_fd, const std::string &text, bool include_sender) const
{
    Channel *channel = _findChannel(chan);
    if (channel)
        _broadcastToChannel(channel, from_fd, text, include_sender);
}

void	Server::_broadcastToChannel(Channel *channel, int from_fd, const std::string &text, bool include_sender) const
{
    if (!channel)
        return;
    const std::set<int> &members = channel->getMembers();
    for (std::set<int>::const_iterator fit = members.begin(); fit != members.end(); ++fit)
    {
        if (!include_sender && *fit == from_fd)
            continue;
        _sendToFD(*fit, text + "\r\n");
    }
}

void	Server::handlePass(Message *msg, User *usr)
{
	std::string	_sPass = this->_password;
	std::list<std::string>	_cPass = msg->getParams();
	std::string	_cmd = msg->getCommand();

	if (_cPass.empty())
	{
		// ERR_NEEDMOREPARAMS
		_reply(usr->getFD(), ERR_NEEDMOREPARAMS, "PASS");
		return ;
	}
	if (_sPass.compare(_cPass.front()) == 0)
	{
		std::cout << "Password match!" << std::endl;
		if (!(usr->isVerified()))
			usr->switchVerification();
		else	 // ERR_ALREADYREGISTERED
		{
			_reply(usr->getFD(), ERR_ALREADYREGISTRED);
		}
		// TODO Server sends some kind of acknowledgment?
	}
	else
	{
		// TODO send ERR back client
		// ERR_PASSWORDMISMATCH
		_reply(usr->getFD(), ERR_PASSWDMISMATCH);
		// TODO disconnect them by implementing the ERROR command
	}
}

// Run through the Messages in the _toProcess queue
// Act on them, delete them.
// TODO Make this spin off thread(s) to process the command efficiently
// NOTE How do we make sure that this is non-blocking?
// TODO Need some kind of matching / switch-case logic here (I guess)
// NOTE This function has friend-based access to Message so it can extract the User involved
// TODO Send messages to client on succesful registration:
// RPL_WELCOME (001),
// RPL_YOURHOST (002),
// RPL_CREATED (003),
// RPL_MYINFO (004),
// at least one RPL_ISUPPORT (005) numeric to the client.
// The server MAY then send other numerics and messages.
// The server SHOULD then respond as though the client sent the LUSERS command and return the appropriate numerics.
// The server MUST then respond as though the client sent it the MOTD command, i.e. it must send either the successful Message of the Day numerics or the ERR_NOMOTD (422) numeric.
// If the user has client modes set on them automatically upon joining the network, the server SHOULD send the client the RPL_UMODEIS (221) reply or a MODE message with the client as target, preferably the former.
void	Server::_processQueue(void)
{
	Message	*do_next;

	while (this->_toProcess.empty() == false)
	{
		do_next = this->_toProcess.front();
		this->_toProcess.pop();
		std::cout << "Processing:" << *do_next << std::endl;
		std::string command = do_next->getCommand();
//		std::cout << "Comando parseado: [" << command << "]" << std::endl;
		if (command.compare("CAP") == 0)
			std::cout << "Ignoring capability negotiation request" << std::endl;
		// Only allow NICK/USER/PASS before registration. All others require full registration.
		else if (!do_next->getOrigin()->isRegistered())
		{
			if (command.compare("PASS") == 0)
				handlePass(do_next, do_next->getOrigin());
			else if (command.compare("NICK") == 0)
			{
				// NICK only allowed after PASS
				if (do_next->getOrigin()->isVerified())
					handleNick(do_next, do_next->getOrigin());
				else
					_reply(do_next->getOrigin()->getFD(), ERR_NOTREGISTERED);
			}
			else if (command.compare("USER") == 0)
			{
				// USER only allowed after PASS
				if (do_next->getOrigin()->isVerified())
					handleUser(do_next, do_next->getOrigin());
				else
					_reply(do_next->getOrigin()->getFD(), ERR_NOTREGISTERED);
			}
			else
			{
				// Send ERR_NOTREGISTERED (451) for other commands until registration completes
				_reply(do_next->getOrigin()->getFD(), ERR_NOTREGISTERED);
			}
		}
		else if (do_next->getOrigin()->isRegistered())
		{
			if (command.compare("PASS") == 0)
				_reply(do_next->getOrigin()->getFD(), ERR_ALREADYREGISTRED);
			if (command.compare("NICK") == 0)
				handleNick(do_next, do_next->getOrigin());
			if (command.compare("USER") == 0)
				handleUser(do_next, do_next->getOrigin());
			if (command.compare("JOIN") == 0)
				handleJoin(do_next, do_next->getOrigin());
			if (command.compare("PART") == 0)
				handlePart(do_next, do_next->getOrigin());
			if (command.compare("NAMES") == 0)
				handleNames(do_next, do_next->getOrigin());
			if (command.compare("LIST") == 0)
				handleList(do_next, do_next->getOrigin());
			if (command.compare("TOPIC") == 0)
				handleTopic(do_next, do_next->getOrigin());
			if (command.compare("INVITE") == 0)
				handleInvite(do_next, do_next->getOrigin());
			if (command.compare("MODE") == 0)
				handleMode(do_next, do_next->getOrigin());
			if (command.compare("PING") == 0)
				handlePing(do_next, do_next->getOrigin());
			if (command.compare("PONG") == 0)
				handlePong(do_next, do_next->getOrigin());
			if (command.compare("QUIT") == 0)
				handleQuit(do_next, do_next->getOrigin());
			if (command.compare("ERROR") == 0)
				handleError(do_next, do_next->getOrigin());
			// FIXME KICK and PRIVMSG are inconsistent with the others, for no good reason
			if (command == "KICK")
				handleKick(do_next, do_next->getOrigin());
			if (command == "PRIVMSG")
				handlePrivmsg(do_next, do_next->getOrigin()->getFD());
			
			// Check if command was not handled by any of the above
			if (command != "PASS" && command != "NICK" && command != "USER" && 
				command != "JOIN" && command != "PART" && command != "NAMES" && 
				command != "LIST" && command != "TOPIC" && command != "INVITE" && 
				command != "MODE" && command != "PING" && command != "PONG" && 
				command != "QUIT" && command != "ERROR" && command != "KICK" && 
				command != "PRIVMSG")
			{
				// Unknown command - send error but don't disconnect
				_reply(do_next->getOrigin()->getFD(), ERR_UNKNOWNCOMMAND, command);
			}
		}
		// deleting the Message here seems to reduce "still reachable" type leaks
		delete do_next;
 	}
}

// Send a numeric reply in response to a command received
// NOTE that this is more easily called if we import a
// bunch of enums in a header, or similar.
// Improved _reply function that sends IRC messages directly
void	Server::_reply(int send_to, int msg_code, const std::string &params) const
{
	std::string message;
	std::stringstream strm;
	
	// Build the IRC message based on the numeric code
	switch (msg_code) {
		// General Errors (OBLIGATORIOS)
		case ERR_NOSUCHNICK:
			message = params + " :No such nick/channel";
			break;
		case ERR_NOSUCHCHANNEL:
			message = params + " :No such channel";
			break;
		case ERR_CANNOTSENDTOCHAN:
			message = params + " :Cannot send to channel";
			break;
		case ERR_TOOMANYCHANNELS:
			message = params + " :You have joined too many channels";
			break;
		case ERR_NORECIPIENT:
			message = ":No recipient given (" + params + ")";
			break;
		case ERR_NOTEXTTOSEND:
			message = ":No text to send";
			break;
		case ERR_UNKNOWNCOMMAND:
			message = params + " :Unknown command";
			break;
			
		// Registration Errors
		case ERR_NONICKNAMEGIVEN:
			message = ":No nickname given";
			break;
		case ERR_ERRONEUSNICKNAME:
			message = params + " :Erroneous nickname";
			break;
		case ERR_NICKNAMEINUSE:
			message = params + " :Nickname is already in use";
			break;
		case ERR_NICKCOLLISION:
			message = params + " :Nickname collision KILL from <user>@<host>";
			break;
		case ERR_UNAVAILRESOURCE:
			message = params + " :Nick/channel is temporarily unavailable";
			break;
		case ERR_NOTREGISTERED:
			message = ":You have not registered";
			break;
		case ERR_NEEDMOREPARAMS:
			message = params + " :Not enough parameters";
			break;
		case ERR_ALREADYREGISTRED:
			message = ":Unauthorized command (already registered)";
			break;
		case ERR_PASSWDMISMATCH:
			message = ":Password incorrect";
			break;
			
		// Channel Errors
		case ERR_USERNOTINCHANNEL:
			message = params + " :They aren't on that channel";
			break;
		case ERR_NOTONCHANNEL:
			message = params + " :You're not on that channel";
			break;
		case ERR_USERONCHANNEL:
			message = params + " :is already on channel";
			break;
		case ERR_CHANNELISFULL:
			message = params + " :Cannot join channel (+l)";
			break;
		case ERR_INVITEONLYCHAN:
			message = params + " :Cannot join channel (+i)";
			break;
		case ERR_BANNEDFROMCHAN:
			message = params + " :Cannot join channel (+b)";
			break;
		case ERR_BADCHANNELKEY:
			message = params + " :Cannot join channel (+k)";
			break;
		case ERR_CHANOPRIVSNEEDED:
			message = params + " :You're not channel operator";
			break;
		case ERR_UNKNOWNMODE:
			message = params + " :is unknown mode char to me for <channel>";
			break;
		case ERR_UMODEUNKNOWNFLAG:
			message = ":Unknown MODE flag";
			break;
			
		// PING/PONG Errors
		case ERR_NOORIGIN:
			message = ":No origin specified";
			break;
		case ERR_NOSUCHSERVER:
			message = params + " :No such server";
			break;
			
		// Additional Channel Errors
		case ERR_YOUREBANNEDCREEP:
			message = ":You are banned from this server";
			break;
		case ERR_KEYSET:
			message = params + " :Channel key already set";
			break;
		case ERR_BADCHANMASK:
			message = params + " :Bad channel mask";
			break;
		case ERR_NOCHANMODES:
			message = params + " :Channel doesn't support modes";
			break;
		case ERR_BANLISTFULL:
			message = params + " :Channel ban list is full";
			break;
			
		// Permission Errors
		case ERR_NOPRIVILEGES:
			message = ":Permission Denied- You're not an IRC operator";
			break;
		case ERR_UNIQOPPRIVSNEEDED:
			message = ":You're not the original channel operator";
			break;
			
		// Welcome Replies (001-004)
		case RPL_WELCOME:
			message = "Welcome to the Internet Relay Network " + params;
			break;
		case RPL_YOURHOST:
			message = "Your host is ft_irc, running version 1.0";
			break;
		case RPL_CREATED:
			message = "This server was created " + _creationTime;
			break;
		case RPL_MYINFO:
			message = "ft_irc 1.0 +itkl";
			break;
			
		// Away Replies (OBLIGATORIOS)
		case RPL_AWAY:
			message = params + " :is away";
			break;
		case RPL_UNAWAY:
			message = ":You are no longer marked as being away";
			break;
		case RPL_NOWAWAY:
			message = ":You have been marked as being away";
			break;
			
		// List Replies
		case RPL_LISTSTART:
			message = "Channel :Users  Name";
			break;
		case RPL_LIST:
		{
			std::stringstream count_stream;
			count_stream << _getChannelMemberCount(params);
			message = params + " " + count_stream.str() + " :" + _getChannelTopic(params);
			break;
		}
		case RPL_LISTEND:
			message = ":End of /LIST";
			break;
			
		// Channel Mode Replies
		case RPL_CHANNELMODEIS:
			message = params + " " + _getChannelModeString(params);
			break;
		case RPL_CREATIONTIME:
			message = params + " " + _getChannelCreationTime(params);
			break;
			
		// Topic Replies
		case RPL_NOTOPIC:
			message = params + " :No topic is set";
			break;
		case RPL_TOPIC:
			message = params + " :" + _getChannelTopic(params);
			break;
		case RPL_TOPICWHOTIME:
			message = params + " " + _getTopicSetter(params) + " " + _getTopicTime(params);
			break;
			
		// Invite Replies
		case RPL_INVITING:
			message = params + " " + _getLastInvitedUser();
			break;
			
		// Names Replies
		case RPL_NAMREPLY:
			message = "= " + params + " :" + _getChannelMembers(params);
			break;
		case RPL_ENDOFNAMES:
			message = params + " :End of /NAMES list";
			break;
			
		default:
		{
			std::stringstream code_stream;
			code_stream << msg_code;
			message = ":Unknown numeric code " + code_stream.str();
			break;
		}
	}
	
	// Create the full IRC message
	strm << MSG_PREFIX_SERVER << " " << msg_code << " " << message << MSG_TERMINATOR;
	
	// Send directly to the user
	_sendToFD(send_to, strm.str());
}

// Helper function to find user by FD
User* Server::_findUserByFD(int fd) const
{
	std::map<int, User*>::const_iterator it = _moreClients.find(fd);
	if (it != _moreClients.end()) {
		return it->second;
	}
	return 0;
}

// Helper function to get channel topic
std::string Server::_getChannelTopic(const std::string &channelName) const
{
	Channel* channel = _findChannel(channelName);
	if (channel) {
		return channel->getTopic();
	}
	return "";
}

// Helper function to get channel members as string
std::string Server::_getChannelMembers(const std::string &channelName) const
{
	Channel* channel = _findChannel(channelName);
	if (!channel) {
		return "";
	}
	
	std::string members;
	const std::set<int>& member_fds = channel->getMembers();
	
	for (std::set<int>::const_iterator it = member_fds.begin(); it != member_fds.end(); ++it) {
		User* user = _findUserByFD(*it);
		if (user) {
			if (channel->isOperator(*it)) {
				members += "@";
			}
			members += user->getNick() + " ";
		}
	}
	
	return members;
}

// Helper function to get channel member count
size_t Server::_getChannelMemberCount(const std::string &channelName) const
{
	Channel* channel = _findChannel(channelName);
	if (channel) {
		return channel->getMemberCount();
	}
	return 0;
}

// Helper function to get last invited user (simplified)
std::string Server::_getLastInvitedUser() const
{
	// This would need to be tracked in your server state
	// For now, return a placeholder
	return "user";
}

// Helper function to get server creation date
std::string Server::_getServerCreationDate() const
{
	return _creationTime;
}

// Helper function to get user real name
std::string Server::_getUserRealName(const std::string &nick) const
{
	User* user = _findUserByNick(nick);
	if (user) {
		return user->getReal();
	}
	return "Unknown";
}

// Helper function to get user channels
std::string Server::_getUserChannels(const std::string &nick) const
{
	User* user = _findUserByNick(nick);
	if (!user) {
		return "";
	}
	
	std::string channels;
	// This would need to be implemented in User class
	// For now, return empty string
	return channels;
}

// Helper function to get channel mode string
std::string Server::_getChannelModeString(const std::string &channelName) const
{
	Channel* channel = _findChannel(channelName);
	if (channel) {
		return channel->getModeString();
	}
	return "";
}

// Helper function to get topic setter
std::string Server::_getTopicSetter(const std::string &channelName) const
{
	(void)channelName; // Suppress unused parameter warning
	// This would need to be tracked in Channel class
	// For now, return a placeholder
	return "admin";
}

// Helper function to get topic time
std::string Server::_getTopicTime(const std::string &channelName) const
{
	(void)channelName; // Suppress unused parameter warning
	// This would need to be tracked in Channel class
	// For now, return a placeholder
	return "1234567890";
}

// Helper function to get channel creation time
std::string Server::_getChannelCreationTime(const std::string &channelName) const
{
	(void)channelName; // Suppress unused parameter warning
	// This would need to be tracked in Channel class
	// For now, return a placeholder
	return "1234567890";
}

// Function to send welcome messages when user completes registration
void Server::sendWelcomeMessages(User *user)
{
	if (!user) return;
	
	// Send RPL_WELCOME
	std::string welcome_msg = user->getNick() + "!" + user->getUser() + "@" + user->getHost();
	_reply(user->getFD(), RPL_WELCOME, welcome_msg);
	
	// Send RPL_YOURHOST
	_reply(user->getFD(), RPL_YOURHOST);
	
	// Send RPL_CREATED
	_reply(user->getFD(), RPL_CREATED);
	
	// Send RPL_MYINFO
	_reply(user->getFD(), RPL_MYINFO);
}

// Check if a nickname is already in use by any connected user
// NOTE This might be faster/scale better if we store (and update) a SET of known nicks
bool	Server::_isNickTaken(const std::string &nick, int except_fd) const
{
    for (std::map<int, User*>::const_iterator it = this->_moreClients.begin(); it != this->_moreClients.end(); ++it)
    {
        if (it->first == except_fd)
            continue;
        User *u = it->second;
        if (u && u->getNick() == nick && !nick.empty())
            return (true);
    }
    return (false);
}

// Helper function, could be moved to Channel or to separate set of functions
// Removes any leading : from the command
// NOTE this should have already happened on Message construction
// Remove any space characters in the name
// (NOTE these should also have been removed in Message construction)
// Returns TRUE if the string has been turned into a valid channel name
// Returns FALSE if not
// NOTE This modifies the string even if we say we can't do anything with it. Bad!
bool	Server::normaliseChanName(std::string *chan)
{
	if (chan->empty())
		return (false);
	
	// Security: Check channel name length
	if (chan->length() > 50)
		return (false);
	
	if (chan->find_first_of(':') == 0)
		chan->erase(0, 1);
	
	// Security: Enhanced forbidden characters check
	if (chan->find_first_of(" ,\a\b\n\r\t\0") != std::string::npos)
		return (false);
	
	// Security: Check for valid channel prefix
	if (chan->empty() || chan->find_first_of("#&") == std::string::npos)
		return (false);
	
	// Security: Additional validation for special characters
	for (size_t i = 0; i < chan->length(); ++i)
	{
		char c = (*chan)[i];
		if (c < 32 || c > 126)
			return (false);
	}
	
	return (true);
}
