#include "Server.hpp"
#include "ACommand.hpp"
#include "Join.hpp"
#include "KickCmd.hpp"
#include "Names.hpp"
#include "Part.hpp"
#include "Ping.hpp"
#include "Privmsg.hpp"
#include "ListCmd.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "Topic.hpp"
#include "Invite.hpp"
#include "Mode.hpp"
#include "Away.hpp"
#include "Who.hpp"
#include "Whois.hpp"
#include "Userhost.hpp"
#include "Pass.hpp"
#include "UserCmd.hpp"
#include "Nick.hpp"
#include "QuitCmd.hpp"
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
// TODO DO we need *both* of these signal includes?
#include <csignal>	// for signal handling
#include <signal.h>	// for signal handling

// Variable estática para controlar el bucle principal del servidor
static volatile sig_atomic_t g_server_running = 1;

// Función manejadora de señales
void signalHandler(int signal)
{
	switch (signal)
	{
		case SIGINT:
			std::cout << "\n[INFO] Señal SIGINT recibida. Cerrando servidor..." << std::endl;
			break;
		case SIGTERM:
			std::cout << "\n[INFO] Señal SIGTERM recibida. Cerrando servidor..." << std::endl;
			break;
		case SIGPIPE:
			std::cout << "\n[INFO] Señal SIGPIPE recibida. Cliente desconectado." << std::endl;
			return; // No cerrar el servidor por SIGPIPE
		default:
			std::cout << "\n[INFO] Señal " << signal << " recibida. Cerrando servidor..." << std::endl;
			break;
	}
	g_server_running = 0;
}

// Set up the Server:
// - create fd for socket
// - set as non-blocking
// - set to listen on IP4, user-supplied port and any incoming IP
// - bind socket to the fd
// - listen on fd
// - Create epoll fd
// - Add socket fd to epoll's listening set
//  SOCK_NONBLOCK   Set the O_NONBLOCK file status flag on the open  file  description
//                (see  open(2)) referred to by the new file descriptor.  Using this
//                flag saves extra calls to fcntl(2) to achieve the same result.
Server::Server(int port, std::string password) : _socketFD(0), _epollFD(0),
												 _serverAddress(), _password(password),
												 _toProcess(), _partial_msgs(),
												 _clients(), _channels(), _creationTime()
{
	std::cout << "Server constructor with parameters called" << std::endl;
	_socketFD = socket(AF_INET, SOCK_STREAM || SOCK_NONBLOCK, 0);
	if (_socketFD == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	std::cout << "Created a socket listening at fd " << _socketFD << std::endl;

	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(port);
	_serverAddress.sin_addr.s_addr = INADDR_ANY;

	std::cout << "Binding...";
	// FIXME If we throw here, the program ends with uncleared memory
	if (bind(_socketFD, (struct sockaddr *)&_serverAddress, sizeof(_serverAddress)) == -1)
	{
		close(_socketFD);
//		delete this;		// NOTE This does not work!
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

	// Configurar manejadores de señales
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGPIPE, signalHandler);

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
		//int clientSocket = accept(this->_socketFD, NULL, NULL);
		// NOTE Must explicitly set socket flags - on Linux they do *not* inherit
		int clientSocket = accept4(this->_socketFD, NULL, NULL, SOCK_NONBLOCK);

		if (clientSocket == -1)
			throw std::runtime_error("Could not get client socket");
		std::cout << "Nuevo cliente conectado, fd: " << clientSocket << std::endl;
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
	// NOTE This is set as static / volatile above.
	// TODO Must be able to explain how that global variable works and is set
	while (g_server_running)
	{
		// returns the number of fds ready for action. -1 signifies an error.
		// "Specifying a timeout of -1 causes  epoll_wait() to block indefinitely,
		// while specifying a timeout equal to zero causes return immediately, even if no events are available"
		int n = epoll_wait(_epollFD, events, MAX_EVENTS, 0);
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
					std::cout << "Mensaje recibido de fd " << events[i].data.fd << ": " << str_buf << std::endl;
					if (!this->_isFullMsg(str_buf))	// buffer does not form a complete message
					{
						std::cout << "Can NOT be parsed, store partial" << std::endl;
						this->_storePartial(events[i].data.fd, str_buf);
//						std::cout << "Done. Stored:" << _partial_msgs[events[i].data.fd] << std::endl;
					}
					// NOTE str_buf might contain multiple \n and we *must* handle that
					else	// Parse into Message and queue for further action
					{
						// With a complete message, must delete partials
						// If there are multiple messages here, parse them *all*
						this->_partial_msgs[events[i].data.fd].erase();
						std::cout << "Can be parsed" << std::endl;
						// NOTE ....What happens if User not found in _clients?
						User*	msgFrom =  this->_clients[events[i].data.fd];
//						while (this->_isFullMsg(str_buf))
						while (!str_buf.empty())
						{
							Message	*nxtMessage = Message::makeMessage(str_buf, msgFrom);
							if (nxtMessage)
								this->_toProcess.push(nxtMessage);
							else
								delete nxtMessage;
							// Strip some text from str_buf
			//				std::cout << "strbuf before:" << str_buf << std::endl;
							str_buf.erase(0, str_buf.find_first_of("\n\r"));
							while (str_buf.find_first_of("\n\r") == 0)
								str_buf.erase(0,1);	// HACK To get rid of the \n at the start now?
							// if (!str_buf.empty())
							// 	std::cout << "strbuf after:" << str_buf << std::endl;
							// else
							// 	std::cout << "strbuf emptied" << std::endl;
						}
					}
					// HACK debugging print statement below
					//this->_printMessageQueue(this->_toProcess);
				}
				// TODO Work out how to handle / merge the 2 different exceptions.
				// - cant parse message- silently ignore or send ERR_NOTENOUGH PARAMS type reply
				// - connection dodgy - is that what they are?
				// NOTE Cannot disconnect a client just because they sent a message that looks too small!
				catch (std::invalid_argument &e)
				{
					std::cerr << e.what() <<std::endl;
//					_removeClient(events[i]);
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

	// Cierre limpio del servidor
	_cleanupServer();
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

// NOTE When the Server destructor is called, all memory freed. This is good!
// FIXME This can be triggered from within ~ACommand. This is very bad!
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

	// Limpiar recursos del servidor
	_cleanupServer();
}

// TODO Consider being more lenient and allowing \r only to terminate commands
bool	Server::_isFullMsg(std::string msg) const
{
	if (msg.empty() || msg.length() < 3)
		return (false);

	size_t	pos = msg.find('\n');
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
	{
		if (new_chars == 0)
			// NOTE Can't throw because this brings the whole server down once a client disconnects with QUIT!
			//throw std::runtime_error("client disconnected");
			std::cerr << "Client disconnected, no exception thrown, was this expected?" << std::endl;
		else
			throw std::runtime_error("failed to read new input");
	}
	else
		ret_val.append(buf);
	return (ret_val);
}

// TODO Consider calling User::normaliseNick here
// (Currently callers use it before sending)
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

// TODO Consider if this needs safety checks before removing the Channel...
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

// FIXME Is anything still using this?
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

// Run through the Messages in the _toProcess queue
// Act on them, delete them.
// TODO Make this spin off thread(s) to process the command efficiently
// NOTE How do we make sure that this is non-blocking?
// TODO This function has friend-based access to Message so it can extract the User involved - still needed?
// NOTE This is what the processQueue *could* look like using ACommands
// DONE Implement matchCmd
// TODO Decide if we are to have an "in" and an "out" queue
// - Get the next message
// - If it is from the Server, send it straight out
// - else, find the command it is
// -- do simple validation
// -- run the command actions
// -- add its output messages back into the queue for processing
// -- delete the command object
void	Server::_processQueue(void)
{
	while (this->_toProcess.empty() == false)
	{
		Message*	do_next;
		do_next = this->_toProcess.front();
		this->_toProcess.pop();
		if (!do_next->getOrigin())
			this->_sendMessage(do_next);
		else
		{
			ACommand*	thingtodo = matchCmd(do_next);
			if (thingtodo)
			{
				if (thingtodo->numParamsOK())
					thingtodo->executeCmd();
				else
				{
					this->_toProcess.push(Message::_reply(*do_next, ERR_NEEDMOREPARAMS));
					return ;
				};
				std::queue<Message *>	to_add = thingtodo->getResponses();
				while (!to_add.empty())
				{
					this->_toProcess.push(to_add.front());
					to_add.pop();
				}
			}
			delete thingtodo;	// Make sure the instance is destroyed after processing.
			delete do_next;	// NOTE in the other branch message was deleted in _sendMessage (I think)
		}
	}
}

// NOTE Does this belong in ACommand?
// No, because the base class is not going to know about all its derived classes.
// - Get command text
// - uppercase it (to work around Hexchat)
// - match against the commands we have defined (unavoidably ugly)
// - return the derived class
// TODO Decide what to do if we don't get a match. Return NULL?
ACommand*	Server::matchCmd(Message* do_next)
{
	std::cout << "Processing:" << *do_next << std::endl;
	std::string command = do_next->getCommand();
	ACommand*	thingtodo = 0;	// TODO Make sure this is the right place to declare this
	// NOTE No origin => message is from us / the server, we send it straight out
	if (command.compare("CAP") == 0)
		std::cout << "Ignoring capability negotiation request" << std::endl;
	// Only allow NICK/USER/PASS before registration. All others require full registration.
	else if (!do_next->getOrigin()->isRegistered())
	{
		do_next->getOrigin()->updateTime();
		if (command.compare("PASS") == 0)
			thingtodo = new Pass(this, *do_next);
		else if ((do_next->getOrigin()->isVerified()) &&
				 (command.compare("NICK") == 0))
			thingtodo = new Nick(this, *do_next);
		else if ((do_next->getOrigin()->isVerified()) &&
				 (command.compare("USER") == 0))
			thingtodo = new UserCmd(this, *do_next);
		else
		{
			this->_toProcess.push(Message::_reply(*do_next, ERR_NOTREGISTERED));
		}
	}
	else if (do_next->getOrigin()->isRegistered())
	{
		do_next->getOrigin()->updateTime();
		if (command.compare("NICK") == 0)
			thingtodo = new Nick(this, *do_next);
		else if (command.compare("USER") == 0)
			thingtodo = new UserCmd(this, *do_next);
		else if (command.compare("JOIN") == 0)
			thingtodo = new Join(this, *do_next);
		else if (command.compare("PART") == 0)
			thingtodo = new Part(this, *do_next);
		else if (command.compare("NAMES") == 0)
			thingtodo = new Names(this, *do_next);
		else if (command.compare("LIST") == 0)
			thingtodo = new ListCmd(this, *do_next);
		else if (command.compare("TOPIC") == 0)
			thingtodo = new Topic(this, *do_next);
		else if (command.compare("INVITE") == 0)
			thingtodo = new Invite(this, *do_next);
		else if (command.compare("MODE") == 0)
			thingtodo = new Mode(this, *do_next);
		else if (command.compare("KICK") == 0)
			thingtodo = new KickCmd(this, *do_next);
		else if (command == "PRIVMSG")
			thingtodo = new Privmsg(this, *do_next);
		else if (command == "PING")
			thingtodo = new Ping(this, *do_next);
		else if (command == "WHO")
			thingtodo = new Who(this, *do_next);
		// HACK Hexchat sends whois in lower case
		else if ((command == "WHOIS") || (command == "whois"))
			thingtodo = new Whois(this, *do_next);
		else if (command == "AWAY")
			thingtodo = new Away(this, *do_next);
		else if (command == "USERHOST")
			thingtodo = new Userhost(this, *do_next);
		else if (command == "QUIT")
			thingtodo = new QuitCmd(this, *do_next);
		else
		{
			thingtodo = NULL;
			this->_toProcess.push(Message::_reply(*do_next, ERR_UNKNOWNCOMMAND));
		}
	}
	// NOTE Uncertain if deleting the message messes up the ACommand! Check it.
//		delete do_next;
	return (thingtodo);
}

// Extract from message:
// - text to be sent
// - length of text
// - fd to where it must be sent
// Ensure that the message is sent correctly.
// TODO Safety checks needed on FD list send_to
// TODO Properly handle the "would block" error
// TODO Quality check on the Message serialization needed?
// FIXME We are not allowed to use errno, remove it!
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
	delete msg_to_send;
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

// Returns a string saying which User modes are supported by us / the Server
// NOTE this is not the same as a User's User Modes!
// All possible modes: https://defs.ircdocs.horse/defs/usermodes.html
// TODO Check which modes we are likely to implement.
// https://modern.ircdocs.horse/#user-modes
// [X] Invisible +i
// [ ] oper +o -- NOTE Not going to use this; there is no "network" beyond
// [X] Local operator +O -- NOTE that these 2 are the same for us
// [X] registered user +r -- not sure about this, we don't have long-lasting accounts
// [ ] WALLOPS +w -- gets WALLOPS messages from server
std::string	Server::getUserModes(void) const
{
	return ("irO");
}

// Returns a string saying which channel modes are supported by us / the Server
// NOTE this is not the same as a channel's channel Modes!
// TODO Check which modes we are likely to implement.
// https://modern.ircdocs.horse/
// [ ] invite only +i
// [ ] limited number of users +l
// [ ] key / password +k
// [ ] ban list +b
// [ ] exceptions to bans +e
// [ ] topic protection +t
std::string	Server::getChanModes(void) const
{
	return ("beIiklt");
}

// Send the WELCOME set of messages to a newly-registered User
// HACK RPL_CREATED and RPL_MYINFO have to use a different path
// ...how to avoid that?
// FIXME Conslidate these WELCOME messages
void	Server::_sendWelcome(Message *msg, User *usr)
{
	(void) usr;	// HACK maybe not ask for this
	this->_toProcess.push(Message::_reply(*msg, RPL_WELCOME));
	this->_toProcess.push(Message::_reply(*msg, RPL_YOURHOST));
	Message* created = Message::_reply(*msg, RPL_CREATED);
	created->addParams(this->getCreation());
	this->_toProcess.push(created);
	Message* info = Message::_reply(*msg, RPL_MYINFO);
	info->addParams(SERVERNAME);
	info->addParams(VERSION);
	info->addParams(this->getUserModes());
	info->addParams(this->getChanModes());
	this->_toProcess.push(info);
}

std::map<std::string, Channel*>	Server::getChannels(void) const
{
	return (this->_channels);
}

Channel*	Server::getChannel(std::string target)
{
	return (this->_channels[target]);
}

bool	Server::checkPasswd(std::string &to_check) const
{
	if (to_check.compare(this->_password) == 0)
		return (true);
	else
		return (false);
}

// Función para limpieza de recursos del servidor
// TODO Check ERROR sending, possibly use ERROR method and direct send method
// - Notify all users
// - close all sockets
// - what about the things remaining in the process queue?
// - what about channels?
void Server::_cleanupServer()
{
	std::cout << "Limpiando recursos del servidor..." << std::endl;

	// Notificar a todos los clientes que el servidor se está cerrando
	for (std::map<int, User*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		User* user = it->second;
		if (user)
		{
			_sendToFD(user->getFD(), "ERROR :Server shutting down\r\n");
		}
	}
	while (!_toProcess.empty())
	{
		Message *tmp = _toProcess.front();
		delete tmp;
		_toProcess.pop();
	}

	// Cerrar todos los sockets de clientes
	for (std::map<int, User*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
	}

	// Cerrar socket del servidor
	if (_socketFD > 0)
	{
		close(_socketFD);
		_socketFD = 0;
	}

	// Cerrar epoll
	if (_epollFD > 0)
	{
		close(_epollFD);
		_epollFD = 0;
	}

	std::cout << "Recursos del servidor limpiados." << std::endl;
}
