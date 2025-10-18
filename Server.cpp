#include "Server.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>	// for close()
#include <sys/epoll.h>
#include <cerrno>	// for error checking in send calls
#include <cstdlib>	// for the EXIT code
#include <cstring>	// for memset. Too many includes!
#include <fcntl.h>	// NOTE Consider not using this, it may be OSX only (see requirements doc)

// Helper para poner un socket en modo no bloqueante
// Set socket to non-blocking by:
// Get the existing flags for the newly-accepted clientSocket
// Add non-blocking to those existing client flags
// Returns FALSE if this fails, caller to handle that
bool Server::_setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return (false);
	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1);
}

// Set up the Server:
// - create fd for socket
// - set as non-blocking
// - set to listen on IP4, user-supplied port and any incoming IP
// - bind socket to the fd
// - listen on fd
// - Create epoll fd
// - Add socket fd to epoll's listening set
Server::Server(int port, std::string password) : _socketFD(0), _epollFD(0),
												 _serverAddress(), _password(password),
												 _toProcess(), _partial_msgs(),
												 _clients(), _channels(), _creationTime()
{
	std::cout << "Server constructor with parameters called" << std::endl;
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFD == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	std::cout << "Created a socket listening at fd " << _socketFD << std::endl;

	if (!_setNonBlocking((_socketFD)))
	{
		close (_socketFD);
		throw std::runtime_error("Cannot make socket nonblocking");
	}

	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	std::cout << "Binding...";
	// FIXME If we throw here, the program ends with uncleared memory
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
	ev.events = EPOLLIN;
	ev.data.fd = _socketFD;
	if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, _socketFD, &ev) == -1)
	{
		close(_socketFD);
		close(_epollFD);
		throw std::runtime_error("Could not add server input to epoll set.");
	}
	this->_creationTime = time(0);
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
		if (!_setNonBlocking(clientSocket))
		{
			close (clientSocket);
			throw std::runtime_error("Failed to set client socket non-blocking!");
		}
		User	*newUser = User::makeUser(clientSocket);
		std::cout << "Created a new user from fd" << clientSocket << std::endl;
		this->_clients[clientSocket] = newUser;
		// Añadir el cliente a epoll
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = clientSocket;
		if (epoll_ctl(_epollFD, EPOLL_CTL_ADD, newUser->getFD(), &ev) == -1)
		{
			// TODO If this fails we should probably remove the User altogether...
			close(clientSocket);
			throw std::runtime_error("Could not add client socket to epoll");
		}
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << "User creation failure" << std::endl;
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
	close(goodbye.data.fd);
	epoll_ctl(_epollFD, EPOLL_CTL_DEL, goodbye.data.fd, NULL);
	this->_partial_msgs.erase(goodbye.data.fd);
	// Free and forget the User instance if present
	std::map<int, User*>::iterator it = this->_clients.find(goodbye.data.fd);
	if (it != this->_clients.end())
	{
		delete it->second;
		this->_clients.erase(it);
	}
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
		// TODO Should the epoll wait error be an exception?
		if (n == -1)
		{
			std::cerr << "epoll_wait error" << std::endl;
			break;
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
						// If there are multiple messages here, parse them *all*
						this->_partial_msgs[events[i].data.fd].erase();
						std::cout << "Can be parsed" << std::endl;
						// NOTE What does the User line do here? Document it.
						// ....What happens if not found in _clients?
						User*	msgFrom =  this->_clients[events[i].data.fd];
						while (this->_isFullMsg(str_buf))
						{
							Message	*nxtMessage = Message::makeMessage(str_buf, msgFrom);
							this->_toProcess.push(nxtMessage);
							// Strip some text from str_buf
//							std::cout << "strbuf before:" << str_buf << std::endl;
							str_buf.erase(0, str_buf.find_first_of('\n'));
							str_buf.erase(0,1);	// HACK To get rid of the \n at the start now?
							// if (!str_buf.empty())
							// 	std::cout << "strbuf after:" << str_buf << std::endl;
							// else
							// 	std::cout << "strbuf emptied" << std::endl;
						}
					}
					std::cout << "Mensaje recibido de fd " << events[i].data.fd << ": " << str_buf << std::endl;
					// HACK debugging print statement below
					//this->_printMessageQueue(this->_toProcess);
				}
				// TODO Work out how to handle / merge the 2 different exceptions.
				// - cant parse message- silently ignore or send ERR_NOTENOUGH PARAMS type reply
				// - connection dodgy - is that what they are?
				catch (std::exception &e)
				{
					std::cerr << e.what() <<std::endl;
					_removeClient(events[i]);
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
	std::cout << "[KICK] Comando recibido de fd " << usr->getFD() << std::endl;
	std::list<std::string> params = msg->getParams();
	if (params.size() < 2)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	std::string chan = params.front(); params.pop_front();
	std::string nick = params.front(); params.pop_front();
	// Optional reason (rest of params)
	std::string reason = "";
	if (!params.empty())
		reason = params.front();
	// Normalise channel name
	if (this->normaliseChanName(&chan) == false)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_BADCHANMASK));
		return ;
	}
	Channel *channel = _findChannel(chan);
	if (!channel)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
		return ;
	}
	// Sender must be channel operator
	if (!channel->isOperator(usr->getFD()))
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_CHANOPRIVSNEEDED));
		return ;
	}
	// Find target user
	User *target = _findUserByNick(nick);
	if (!target)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHNICK));
		return ;
	}
	// Target must be member of channel
	if (!channel->isMember(target))
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_USERNOTINCHANNEL));
		return ;
	}
	// Remove target from channel
	channel->removeMember(target);
	target->removeChannel(chan);	// NOTE This depends on User storing their Channels, which they don't, currently
	if (reason.empty()) reason = "Kicked";
	// Notify channel and target
	_broadcastToChannel(channel, -1, ":server KICK " + chan + " " + nick + " :" + reason, true);
	_sendToFD(target->getFD(), ":server KICK " + chan + " " + nick + " :" + reason + "\r\n");
}

// Aquí va la lógica para enviar mensajes privados o a canales
// TODO There are further checks needed on whether a message is allowed, see docs
// Sends a message to user(s) or channel(s)
// https://modern.ircdocs.horse/#privmsg-message
// FIXME Mesage formatting is wrong. sender? Message must be precede by :
void Server::handlePrivmsg(Message *msg, User *usr)
{
	std::list<std::string> params = msg->getParams();
	if (params.size() < 2)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	std::string target = params.front();
	params.pop_front();
	std::string text = params.front();
	// Channel message
	if (!target.empty() && target[0] == '#')
	{
		// Only allow if sender is member of the channel
		Channel *channel = _findChannel(target);
		if (!channel)
		{
			this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
			return ;
		}
		// TODO Faster to find Channel membership directly with User? Or not?
		if (!channel->isMember(usr))
		{
			// 404 ERR_CANNOTSENDTOCHAN
			this->_toProcess.push(Message::_reply(*msg, ERR_CANNOTSENDTOCHAN));
			return ;
		}
		else
			_toProcess.push(Message::_channelMessage(*msg, channel));
	}
	else
	{	// For individual user
		// TODO Need to change the text format in PRIVMSG e.g. source, or not?
		User *to = _findUserByNick(target);
		if (to)
		{
			// FIXME This won't work without yet another overload. (to what?)
//			_toProcess.push(Message::_replyNonNumeric(*msg, to));
		}
		//	_sendToFD(to->getFD(), text + "\r\n");
		else
			this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHNICK));
	}
}

// Get user
// Get parameters
// Check the requested nick is valid and does not already exist
// Set new nickname on User (will need a setter on User?)
// FIXME IF the nick name is already in use that doesn't seem to stop registration?
// TODO Acknowledge successful NICK:
// The NICK message may be sent from the server to clients to acknowledge their
// NICK command was successful, and to inform other clients about the change of nickname.
// In these cases, the <source> of the message will be the old nickname
// [ [ "!" user ] "@" host ] of the user who is changing their nickname.
void	Server::handleNick(Message *msg, User *usr)
{
	std::list<std::string>	_params = msg->getParams();
	if (_params.empty())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NONICKNAMEGIVEN));
		return ;
	}
	std::string	newNick = _params.front();
	std::cout << "Trying to set nickname to " << newNick << std::endl;
	// NOTE These characters are forbidden from starting the nick
	std::string	notLeading = "#:&123456789";
	std::string	forbidden = " \b\n\r";

	if ((newNick.find_first_of(notLeading) == 0) ||
		(newNick.find_first_of(forbidden) != std::string::npos))
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_ERRONEUSNICKNAME));
	}
	else if (_isNickTaken(newNick, usr->getFD()))
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NICKNAMEINUSE));
	}
	else
	{
		std::cout << "setting nickname to " << newNick << std::endl;
		bool wasRegistered = usr->isRegistered();
		usr->setNick(newNick);
		// If user just became registered (had USER but was missing NICK), send welcome
		if (!wasRegistered && usr->isRegistered())
		{
			this->_toProcess.push(Message::_reply(*msg, RPL_WELCOME));
			this->_toProcess.push(Message::_reply(*msg, RPL_YOURHOST));
			this->_toProcess.push(Message::_reply(*msg, RPL_CREATED));
			this->_toProcess.push(Message::_reply(*msg, RPL_MYINFO));
		}
	}
}

// FIXME IF the nick name is already in use that doesn't seem to stop registration?
// TODO Sure there are other errors to catch here...
// FIXME Hexchat at least does not get given a Real Name
void	Server::handleUser(Message *msg, User *usr)
{
	if (usr->isRegistered())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_ALREADYREGISTERED));
		return ;
	}
	std::list<std::string>	_params = msg->getParams();
	if ((_params.size() != 4) || (_params.front().empty()))
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	std::string	newUser = _params.front();
	// Skip to the final entry (used the first, ignore the middle 2)
	_params.pop_front();
	_params.pop_front();
	_params.pop_front();
	std::string	newRName = _params.front();
	if (newUser.empty())
		newUser = usr->getNick();
	if (newRName.empty())
		newRName = newUser;
	usr->setUser(newUser);
	usr->setReal(newRName);
	std::cout << "User: " << newUser << ", Really: " << newRName << std::endl;
	// Only send welcome bundle if user is fully registered (has valid nickname)
	if (usr->isRegistered())
	{
		this->_toProcess.push(Message::_reply(*msg, RPL_WELCOME));
		this->_toProcess.push(Message::_reply(*msg, RPL_YOURHOST));
		this->_toProcess.push(Message::_reply(*msg, RPL_CREATED));
		this->_toProcess.push(Message::_reply(*msg, RPL_MYINFO));
	}
}

// FIXME This does not cause clients to realise they have joined a room :|
// TODO Send acknowledgements per https://modern.ircdocs.horse/#join-message
// [ ] A JOIN message with the client as the message <source> and the channel they have joined as the first parameter of the message.
// [X] The channel’s topic (with RPL_TOPIC (332) and optionally RPL_TOPICWHOTIME (333)), and no message if the channel does not have a topic.
// [ ] A list of users currently joined to the channel (with one or more RPL_NAMREPLY (353) numerics followed by a single RPL_ENDOFNAMES (366) numeric). These RPL_NAMREPLY messages sent by the server MUST include the requesting client that has just joined the channel.
// DONE Break out the name normalisation to a helper function
// TODO JOIN can accept an alternative parameter of '0'
// TODO Improve parameter handling so JOIN Can handle multiple Channels
// FIXME the reply or broadcast message repeats the #channelname
void	Server::handleJoin(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
        return ;
	}
    std::string chan = params.front();
    // If the channel name is valid, store and remove from our params
	if (!this->normaliseChanName(&chan))
	{
		// Send error message and stop processing message
		this->_toProcess.push(Message::_reply(*msg, ERR_BADCHANMASK));
		return ;
	}
	else
    	params.pop_front();

	// If the channel cannot be found, create it
    Channel *channel = this->_findChannel(chan);
    if (!channel)
        channel = this->_createChannel(chan);

	// NOTE This logic is odd, why remove an invite? Just to keep the list clean?
    if (channel->isInviteOnly())
    {
        if (!channel->isInvited(usr->getNick()))
        {
			this->_toProcess.push(Message::_reply(*msg, ERR_INVITEONLYCHAN, channel));
            return ;
        }
        else
        {
            channel->removeInvite(usr->getNick());
        }
    }

	// Add member to channel
    if (channel->addMember(usr))
    {
        usr->addChannel(chan);
		// Send JOIN confirmation
		this->_toProcess.push(Message::_replyNonNumeric(*msg, channel));

		// Send topic if channel has one
		if (!channel->getTopic().empty())
		{
			this->_toProcess.push(Message::_reply(*msg, RPL_TOPIC, channel));
			this->_toProcess.push(Message::_reply(*msg, RPL_TOPICWHOTIME, channel));
		}

		// Send names list (shows who is in the channel)
		this->_toProcess.push(Message::_reply(*msg, RPL_NAMREPLY, channel));
		this->_toProcess.push(Message::_reply(*msg, RPL_ENDOFNAMES, channel));

        // Notify channel (simple join message, or should it be a NOTICE?)
		this->_toProcess.push(Message::Message::_channelMessage(*msg, channel));
    }
	return ;
}

// DONE Factor out the channel name normalisation, it is repeated everywhere
// TODO Unsuccessful commands will need an Error reply
// TODO Use a standard function to craft message, not hardcoded parameters
// TODO Will need to be able to handle *multiple* Channel PARTs
void	Server::handlePart(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
        return ;
    std::string chan = params.front();
	if (!this->normaliseChanName(&chan))
	{
		// Send error message and stop processing message
		this->_toProcess.push(Message::_reply(*msg, ERR_BADCHANMASK));
		return ;
	}
	else
    	params.pop_front();

    Channel *channel = _findChannel(chan);
    if (!channel)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
        return ;
	}

    if (channel->removeMember(usr))
    {
		// Send a PART confirmation to the User
		this->_toProcess.push(Message::_replyNonNumeric(*msg, channel));
        usr->removeChannel(chan);
		// TODO Send NOTICE to that channel using Message and queue
		this->_toProcess.push(Message::_channelMessage(*msg, channel));
        //_broadcastToChannel(channel, -1, ":server NOTICE " + chan + " :" + usr->getNick() + " left", true);

        // If channel is empty, remove it
        if (channel->isEmpty())
            _removeChannel(chan);
    }
	else
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOTONCHANNEL));
		return ;
	}
}

// FIXME Does not comply with specifications of NAMES command
// https://modern.ircdocs.horse/#names-message
// TODO Read msg parameters and call to each named channel
// void	Server::handleNames(Message *msg, User *usr)
// {
//     (void)msg;
//     for (std::map<std::string, Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
//     {
//         const std::string &chan = it->first;
//         std::string line = "353 = " + chan + " :";
//         Channel *c = it->second;
// 		// FIXME here this won't work with Users returned...
//         const std::set<int> &members = c->getMembers();
//         for (std::set<int>::const_iterator fit = members.begin(); fit != members.end(); ++fit)
//         {
//             std::map<int, User*>::const_iterator uit = _clients.find(*fit);
//             if (uit != _clients.end() && uit->second)
//             {
//                 if (fit != members.begin()) line += " ";
//                 line += uit->second->getNick();
//             }
//         }
//         _sendToFD(usr->getFD(), line + "\r\n");
//     }
// }

// TODO This should handle a list of channels
// TODO Filter the channel list that we call before looping over and listing
void	Server::handleList(Message *msg, User *usr)
{
	(void) usr;	// HACK surely we need this?
	if (msg->getParams().size() == 0)	// list all channels
	{
		for (std::map<std::string, Channel*>::const_iterator it = _channels.begin(); it != _channels.end(); ++it)
		{
			Channel *c = it->second;
			this->_toProcess.push(Message::_reply(*msg, RPL_LIST, c));
		}
		this->_toProcess.push(Message::_reply(*msg, RPL_LISTEND));
	}
	else
		std::cerr << "LIST with selected channels not implemented yet" << std::endl;
}

// FIXME Blank topic (RPL_TOPIC?) does not supply channel name (KVIRC)
// The broadcast message must include the channel name (and goes to all in channel)
// FIXME Parsing not putting the : in the correct place (sometimes?)
void	Server::handleTopic(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.empty())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return;
	}
    std::string chan = params.front();
    Channel *channel = _findChannel(chan);
    if (!channel)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
		return;
	}
    if (params.size() == 1)
    {
		this->_toProcess.push(Message::_reply(*msg, RPL_TOPIC));
        return ;
    }
    if (channel->isTopicProtected() && !channel->isOperator(usr->getFD()))
    {
		this->_toProcess.push(Message::_reply(*msg, ERR_CHANOPRIVSNEEDED));
        return ;
    }
    params.pop_front();
    std::string newTopic = params.front();
    channel->setTopic(newTopic, usr->getNick());
	this->_toProcess.push(Message::_channelMessage(*msg, channel));
}

void	Server::handleInvite(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    if (params.size() < 2) return ;
    std::string nick = params.front(); params.pop_front();
    std::string chan = params.front();
    Channel *channel = _findChannel(chan);
    if (!channel) return ;

    if (!channel->isOperator(usr->getFD()))
    {
        _sendToFD(usr->getFD(), ":server 482 " + chan + " :You're not channel operator\r\n");
        return ;
    }
    channel->addInvite(nick);
    User *target = _findUserByNick(nick);
    if (target)
        _sendToFD(target->getFD(), ":server INVITE " + nick + " " + chan + "\r\n");
}

// TODO Handle modestring-less commands with a reply
// - param 1 = target, either Nick or Channel
// - param 2 = optional modestring
// - param 3 = optional mode arguments
void	Server::handleMode(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
	if (params.empty())
	{
        this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
        return ;
	}
    std::string chan = params.front();
	params.pop_front();
	// FIXME The first parameter could also be a USER, how do we handle that?
    Channel *channel = _findChannel(chan);
    if (!channel)
	{
		// TODO Add channel name to this error? Check specification
        this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
		return ;
	}
	if (params.empty())
	{
		// We only got a channel so we list the modes and return
		// RPL_CHANNELMODEIS (324)
		// "<client> <channel> <modestring> <mode arguments>..."
		// Followed by the creation time
		// RPL_CREATIONTIME
		// <client> <channel> <creationtime>" (with timestamp)
		this->_toProcess.push(Message::_reply(*msg, RPL_CREATIONTIME));
		return ;
	}
	// NOTE Below here only if more than 1 param was given
	// NOTE No privileges needed to get a listing, but from here we change things
    if (!channel->isOperator(usr->getFD()))
    {
        this->_toProcess.push(Message::_reply(*msg, ERR_CHANOPRIVSNEEDED, channel));
        return ;
    }
    std::string flags = params.front(); params.pop_front();
	// FIXME This mode-making logic must be shared, else it will cause problems
    bool adding = true;
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
            std::string modeStr = (adding ? "+" : "-") + std::string(1, f);
            if (!param.empty())
                modeStr += " " + param;
			this->_toProcess.push(Message::_channelMessage(*msg, channel));
            //_broadcastToChannel(channel, -1, ":server MODE " + chan + " " + modeStr, true);
        }
    }
}

// Client says PING <token> then we return PONG <token>
// As PONG only comes back in reponse to PING,
// if we don't *send* a PING then there's no need to handle PONG
// TODO Use usr to update a "last seen" value for AWAY, autodisconnects, etc
void	Server::handlePing(Message *msg, User *usr)
{
	(void) usr;
    std::list<std::string> params = msg->getParams();
    if (params.empty())
    {
        this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
        return ;
    }
    std::string origin = params.front();
    if (origin.empty())
    {
        this->_toProcess.push(Message::_reply(*msg, ERR_NOORIGIN));
        return ;
    }
	// HACK send NULL for lack of overload, dangerous!
	this->_toProcess.push(Message::_replyNonNumeric(*msg));
}

// NOTE When the Server destructor is called, all memory freed. This is good!
Server::~Server(void)
{
	// Libera recursos si es necesario
	std::cout << "Server destructor called." << std::endl;

	// Clean up channels
	for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		delete it->second;
	}
	_channels.clear();

	// Clean up users
	for (std::map<int, User*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		delete it->second;
	}
	_clients.clear();
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
	new_chars = read(fd, buf, (sizeof(buf) - 1 - chars_already));
	if (new_chars <= 0)
		throw std::runtime_error("failed to read new input");
//		_removeClient(events[i]);
	else
		ret_val.append(buf);
	return (ret_val);
}

User* Server::_findUserByNick(const std::string &nick) const
{
    for (std::map<int, User*>::const_iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
    {
        if (it->second && it->second->getNick() == nick)
            return it->second;
    }
    return NULL;
}

// TODO Consider making this a public Channel method instead
Channel* Server::_findChannel(const std::string &name) const
{
    std::map<std::string, Channel*>::const_iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    return NULL;
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
    write(fd, text.c_str(), text.size());
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
	// HACK for compilation
	(void) from_fd;
	(void) text;
	(void) include_sender;
	// FIXME here this won't work with Users returned...
    // const std::set<int> &members = channel->getMembers();
    // for (std::set<int>::const_iterator fit = members.begin(); fit != members.end(); ++fit)
    // {
    //     if (!include_sender && *fit == from_fd)
    //         continue;
    //     _sendToFD(*fit, text + "\r\n");
    // }
}

void	Server::handlePass(Message *msg, User *usr)
{
	std::string	_sPass = this->_password;
	std::list<std::string>	_cPass = msg->getParams();
	std::string	_cmd = msg->getCommand();

	if (_cPass.empty())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	if (_sPass.compare(_cPass.front()) == 0)
	{
		std::cout << "Password match!" << std::endl;
		if (!(usr->isVerified()))
			usr->switchVerification();
		else
		{
			this->_toProcess.push(Message::_reply(*msg, ERR_ALREADYREGISTERED));
		}
		// TODO Server sends some kind of acknowledgment?
	}
	else
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_PASSWORDMISMATCH));
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
// FIXED Segfaults if given a Server numeric reply - bypass the checks?
// NOTE This is focused a lot on actions the Server must do.
// ...sometimes all that needs to happen is to send a reply...
// TODO implement WHO command
// TODO implement USERHOST command
// https://modern.ircdocs.horse/#userhost-message
// TODO implement QUIT command (port from branch)
void	Server::_processQueue(void)
{
	Message*	do_next;

	while (this->_toProcess.empty() == false)
	{
		do_next = this->_toProcess.front();
		this->_toProcess.pop();
		std::cout << "Processing:" << *do_next << std::endl;
		std::string command = do_next->getCommand();
//		std::cout << "Comando parseado: [" << command << "]" << std::endl;
		// NOTE No origin => message is from us / the server, we send it straight out
		if (!do_next->getOrigin())
			this->_sendMessage(do_next);
		else if (command.compare("CAP") == 0)
			std::cout << "Ignoring capability negotiation request" << std::endl;
		// Only allow NICK/USER/PASS before registration. All others require full registration.
		// NOTE This check should not be done with Server messages.
		else if (!do_next->getOrigin()->isRegistered())
		{
			do_next->getOrigin()->updateTime();
			if (command.compare("PASS") == 0)
				handlePass(do_next, do_next->getOrigin());
			else if ((do_next->getOrigin()->isVerified()) &&
					  (command.compare("NICK") == 0))
				handleNick(do_next, do_next->getOrigin());
			else if ((do_next->getOrigin()->isVerified()) &&
					 (command.compare("USER") == 0))
				handleUser(do_next, do_next->getOrigin());
			else
			{
				this->_toProcess.push(Message::_reply(*do_next, ERR_NOTREGISTERED));
			}
		}
		else if (do_next->getOrigin()->isRegistered())
		{
			do_next->getOrigin()->updateTime();
			if (command.compare("NICK") == 0)
				handleNick(do_next, do_next->getOrigin());
			else if (command.compare("USER") == 0)
				handleUser(do_next, do_next->getOrigin());
			else if (command.compare("JOIN") == 0)
				handleJoin(do_next, do_next->getOrigin());
			else if (command.compare("PART") == 0)
				handlePart(do_next, do_next->getOrigin());
			// HACK for compilation pending function fix
			// else if (command.compare("NAMES") == 0)
			// 	handleNames(do_next, do_next->getOrigin());
			else if (command.compare("LIST") == 0)
				handleList(do_next, do_next->getOrigin());
			else if (command.compare("TOPIC") == 0)
				handleTopic(do_next, do_next->getOrigin());
			else if (command.compare("INVITE") == 0)
				handleInvite(do_next, do_next->getOrigin());
			else if (command.compare("MODE") == 0)
				handleMode(do_next, do_next->getOrigin());
			else if (command.compare("KICK") == 0)
				handleKick(do_next, do_next->getOrigin());
			else if (command == "PRIVMSG")
				handlePrivmsg(do_next, do_next->getOrigin());
			else if (command == "PING")
				handlePing(do_next, do_next->getOrigin());
			else if (command == "WHO")
				handleWho(do_next, do_next->getOrigin());
			else if (command == "AWAY")
				handleAway(do_next, do_next->getOrigin());
			else if (command == "USERHOST")
				handleUserhost(do_next, do_next->getOrigin());
			else if (command == "QUIT")
				handleQuit(do_next, do_next->getOrigin());
			else
				this->_toProcess.push(Message::_reply(*do_next, ERR_UNKNOWNCOMMAND));
		}
		// NOTE deleting the Message here seems to reduce "still reachable" type leaks
		delete do_next;
 	}
}

// No, or empty parameter = NOT away
// otherwise: going away, broadcast message
// TODO Handle going-away message (e.g. broadcast to channel)
void	Server::handleAway(Message *msg, User *usr)
{
	if (msg->getParams().empty())
	{
		usr->setAway(false);
		this->_toProcess.push(Message::_reply(*msg, RPL_UNAWAY));
	}

	else
	{
		usr->setAway(true);
		this->_toProcess.push(Message::_reply(*msg, RPL_NOWAWAY));
	}
}

// On quit, send ERROR to the client
// broadcast QUIT to their channels
// remove them from all channels and clean up traces
// NOTE The ERROR probably has to act directly as the FD will disappear...
// TODO Test (refactor?) the user-removal logic
// - all channels (should be encapsulated in removeMember method)
// - Server listings (perhaps roll into ERROR)
// FIXME _removeUser() and _removeCLient() are too similar, confusing
void	Server::handleQuit(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    std::string reason ("Quit: " + params.back());

//    std::string quitMsg = ":" + usr->getNick() + " QUIT :" + reason;
//    std::string quitMsg = byethen->serialiseMsg();

    // Get all channels the user is on and broadcast QUIT message
    // TODO The channel collection should be when filling the list of FDs
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        Channel *channel = it->second;
        if (channel->isMember(usr))
        {
			Message *broadcast = Message::_channelMessage(*msg, channel);
			broadcast->addParams(reason);
			this->_toProcess.push(broadcast);
//            _broadcastToChannel(channel, usr->getFD(), quitMsg, false);
            channel->removeMember(usr);
        }
    }
	// Then we call handleError to remove the User themselves from the Server
	this->handleError(msg, usr);
}

// The parameter is either a NICK or a Channel name (we can ignore wildcards)
// Reply with multiple 352 terminated by RPL_ENDOFWHO (315)
void	Server::handleWho(Message *msg, User *usr)
{
	std::list<std::string>	params = msg->getParams();
	if (params.empty())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	std::string	mask = params.front();
	(void) usr;	// HACK for compilation
	if (mask.empty())
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	if (mask.find_first_of("#&") == 0)
	{
		// treat as Channel. Return all members of that Channel
		Channel*	target = this->_channels[mask];
		if (!target)
			std::cerr << "Oops channel not found what we do?" << std::endl;
		else
		{
			std::set<User *>	users = target->getMembers();
			std::set<User *>::const_iterator it = users.begin();
			while (it != users.end())
			{
				User*	user = *it;
			 	this->_toProcess.push(Message::_reply(*msg, RPL_WHOREPLY, user));
				it++;
			}
//			std::cerr << "WHO for channels not implemented yet" << std::endl;
			// // FIXME Needs channel in the params, this solution doesnt cut it
			// // FIXME This is a C++11 form
			// for (User* user : users)
			// {
			// 	this->_toProcess.push(Message::_reply(*msg, RPL_WHOREPLY, user));
			// }
		}
	}
	else // treating it as a NICK
	{
		User*	user = this->_findUserByNick(mask);
		if (!user)
			this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHNICK, user));
		else
			this->_toProcess.push(Message::_reply(*msg, RPL_WHOREPLY, user));
	}
	// send final 315
	this->_toProcess.push(Message::_reply(*msg, RPL_ENDOFWHO));
}


// Extract from message:
// - text to be sent
// - length of text
// - fd to where it must be sent
// Ensure that the message is sent correctly.
// TODO Safety checks needed on FD list send_to
// TODO Properly handle the "would block" error
// TODO Quality check on the Message serialization needed?
void	Server::_sendMessage(Message *msg_to_send) const
{
	std::string	msg_as_str = msg_to_send->serialiseMsg();
	// const here is to avoid -fpermissive compiler warning
	const char*	msg_buf = msg_as_str.c_str();
	size_t			str_len = msg_as_str.length();
	std::list<int>	send_to = msg_to_send->getTargets();

	while (!send_to.empty())
	{
		int	send_nxt = send_to.front();
		std::cout << "Sending:" << msg_buf << std::endl;
		if (send(send_nxt, msg_buf, str_len, MSG_DONTWAIT) == -1)
		{
			// check error number and handle it
			switch (errno)
			{
				case EWOULDBLOCK:
					std::cerr << "Would block, split message or drop it" << std::endl;
					break ;
				default:
					std::cerr << "Dunno, something else went wrong" << errno << std::endl;
			}
		}
		else
		{
			std::cout << "Server reply message sent OK" << std::endl;
		}
		// move through the list - does this handle memory OK?
		send_to.pop_front();
	}
}

// Check if a nickname is already in use by any connected user
// NOTE This might be faster/scale better if we store (and update) a SET of known nicks
// TODO Test this, does it ever return false? Very hard to read.
bool	Server::_isNickTaken(const std::string &nick, int except_fd) const
{
    for (std::map<int, User*>::const_iterator it = this->_clients.begin(); it != this->_clients.end(); ++it)
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
// Returns TRUE if the string has been turned into a valid channel name
// Returns FALSE if not
// NOTE This modifies the string even if we say we can't do anything with it. Bad!
bool	Server::normaliseChanName(std::string *chan)
{
	if (chan->empty())
		return (false);
	if (chan->find_first_of(':') == 0)
		chan->erase(0, 1);
	// Channel must start with # or &
	if (chan->empty() || ((*chan)[0] != '#' && (*chan)[0] != '&'))
		return (false);
	// Channel must have at least one character after the # or &
	if (chan->length() < 2)
		return (false);
	// Reject ## or && at the start (invalid channel names)
	if (chan->length() >= 2 && (*chan)[0] == (*chan)[1])
		return (false);
	// These characters are forbidden in Channel names
	if (chan->find_first_of(" ,\a") != std::string::npos)
		return (false);
	return (true);
}

// FIXME This should be of the type "4 days, 3 hours 10 minutes 15 seconds"
std::string	Server::getUptime(void) const
{
	time_t	uptime = difftime(time(0), this->_creationTime);
	struct tm *timeinfo = localtime(&uptime);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a %b %d %Y at %H:%M:%S %Z", timeinfo);
	return (std::string(buffer));
}

std::string	Server::getCreation(void) const
{
	time_t	cretime = this->_creationTime;
	struct tm *timeinfo = localtime(&cretime);
	std::string	creationmsg = "Server running since: ";
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%a %b %d %Y at %H:%M:%S %Z", timeinfo);
	creationmsg.append(buffer);
	return (creationmsg);
}

// Return RPL_USERHOST 302 for up to 5 NICKs
// This is one list with a space-spearated parameter list of the userHostMsg ouptuts
// TODO Test with multiple NICKs
void	Server::handleUserhost(Message *msg, User *usr)
{
	(void) usr;	// HACK this should be used to check permission to view nicks
	std::list<std::string>	in_params = msg->getParams();
	std::list<std::string>	o_params;
	if (in_params.size() > 5)
		return ;	// Silently ignore
	else
	{
		while (!in_params.empty())
		{
			std::string	nick = in_params.front();
			// Find User by Nick
			User*	target = this->_findUserByNick(nick);
			// TODO This should *do* something with the return!
			if (target)
				o_params.push_back(target->getUserHostMsg());
			in_params.pop_front();
		}
		Message* reply;
		reply = Message::_replyNonNumeric(*msg);
		// TODO add o_params to reply
		reply->addParams(o_params);
		this->_toProcess.push(reply);
	}
}

// Used only in case of unrecoverable errors, or in response to the User sends QUIT
// i.e. We do not expect to *receive* this ever, so it does not have to be checked for!
// BUT Do we want to process it ourselves from the queue?
// Best to not remove the User until *after* we send this message
// The only parameter needed is a disconnection reason - given by client or us (e.g. timed out)
// TODO Adapt this to handle Server-initiated disconnections
// - Server-determined errorMsg
// - Triggered by conditions like Away too long
// ...implies new/different parameters needed.
// FIXED Duplicate parameters in message
void	Server::handleError(Message *msg, User *usr)
{
    std::list<std::string> params = msg->getParams();
    std::string errorMsg = "";

    if (!params.empty())
    {
        errorMsg = params.front();
		// TODO What is this checking trying to achieve?
        if (!errorMsg.empty() && errorMsg[0] == ':')
            errorMsg.erase(0, 1);
    }
    if (errorMsg.empty())
        errorMsg = "Connection closed by client";

    // Log the error
    std::cerr << "ERROR from " << usr->getNick() << ": " << errorMsg << std::endl;

    // Remove user from all channels
    // TODO Bypass this if we have come from QUIT command, already done.
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        Channel *channel = it->second;
        if (channel->isMember(usr))
        {
            channel->removeMember(usr);
        }
    }
    // Make error Message *and *send* *immediately
    // (If we wait, things might get out of order)
    Message* bye = Message::_replyNonNumeric(*msg);
	bye->addParams(errorMsg);
	std::cout << "Direct send of Error message:" << *bye << std::endl;
	this->_sendMessage(bye);
    // Remove user from server
	this->_removeUser(*usr);
}

// Removes the User from the Server
// TO be called from QUIT and / or ERROR
// NOTE Must have already removed from all their Channels, else will cause bother
void	Server::_removeUser(User &usr)
{
	int	usr_FD = usr.getFD();

	// First we stop listening to avoid accidental reconnection
    epoll_ctl(_epollFD, EPOLL_CTL_DEL, usr_FD, NULL);
    _partial_msgs.erase(usr_FD);
    _clients.erase(usr_FD);

    // Close the connection
    close(usr_FD);

    // Delete user object
    // FIXME HACK does not work called like this
//    delete usr;
}
