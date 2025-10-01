#include "Server.hpp"
#include "Message.hpp"
#include "User.hpp"
#include "Channel.hpp"
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
												 _toProcess(), _clients(), _partial_msgs(),
												 _moreClients(), _channels()
{
	std::cout << "Server constructor with parameters called" << std::endl;
	_socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (_socketFD == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	std::cout << "Created a socket listening at fd " << _socketFD << std::endl;

	// Set socket to non-blocking by:
	// Get the existing flags for the newly-created Socket
	int flags = fcntl(_socketFD, F_GETFL, 0);
	// Add non-blocking to those existing client flags
	fcntl(_socketFD, F_SETFL, flags | O_NONBLOCK);

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
}

// What needs to be done when we get the first contact from a new client
// - Accept
// - set the socket as non-blocking
// - make an event struct for its input
// - add that to the epollFD that we monitor.
// NOTE If something goes wrong, throws a runtime_error exception to be caught outside
// TODO Make proper use of the newUser!
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
//		delete newUser;	// HACK Until I know what to do with the user
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
		std::cerr << e.what() << "User creation failure" << std::endl;
	}
}

// Small wrapper for dealing with a client that has disconnected.
// - close its fd
// - remove that fd from the epoll listening set
// NOTE Should the events array we pass in be stored as part of the class instead?
// NOTE This could work on an int fd only, but this allows other actions if needed.
// DONE Once we have User instances, this should remove those too
void	Server::_removeClient(struct epoll_event &goodbye)
{
	std::cout << "Cliente desconectado, fd: " << goodbye.data.fd << std::endl;
	close(goodbye.data.fd);
	epoll_ctl(_epollFD, EPOLL_CTL_DEL, goodbye.data.fd, NULL);
	_clients.erase(goodbye.data.fd);
	this->_partial_msgs.erase(goodbye.data.fd);
	// Free and forget the User instance if present
	std::map<int, User*>::iterator it = this->_moreClients.find(goodbye.data.fd);
	if (it != this->_moreClients.end())
	{
		delete it->second;
		this->_moreClients.erase(it);
	}
}

// Activates the Server's epoll loop
// - Waits for an event
// -- if the event is on the Server fd, we treat it as a new connection
// --- i.e. we add it to the set of Things Listened For
// --- first setting it as nonblocking
// -- if it is *not* on the server fd then it is from a client
// --- TODO Do more useful things with the client input - build command, register users
// --- (Currently we just echo the input to stdout)
// TODO Some parts of this should be made more C++ like,
// e.g. stringstream insteaad of manually terminating a character buffer
// TODO The errors should throw exception of some kind
// TODO Refactor this into separate functions; it is unreadable now
// TODO Handle client disconnnections better: currently cause "failed to read new input"
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
						this->_partial_msgs[events[i].data.fd].erase();
						std::cout << "Can be parsed" << std::endl;
						User*	msgFrom =  this->_moreClients[events[i].data.fd];
						Message	*nxtMessage = Message::makeMessage(str_buf, msgFrom);
						this->_toProcess.push(nxtMessage);
					}
					std::cout << "Mensaje recibido de fd " << events[i].data.fd << ": " << str_buf << std::endl;
					// HACK Loop to send to all other clients connected
					for (std::set<int>::iterator it = _clients.begin(); it != _clients.end(); ++it)
					{
						if (*it != events[i].data.fd)
							write(*it, str_buf.c_str(), str_buf.size());
					}
					// HACK debugging print statement below
					//this->_printMessageQueue(this->_toProcess);
				}
				// TODO Work out how to handle / merge the 2 different exceptions.
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
void Server::handleKick(Message *msg, int sender_fd)
{
	// Aquí va la lógica para expulsar a un usuario de un canal
	// Ejemplo: obtener parámetros, buscar usuario, eliminarlo del canal, notificar, etc.
	(void) msg;
	std::cout << "[KICK] Comando recibido de fd " << sender_fd << std::endl;
	// Puedes acceder a los parámetros con msg->getParams()
}

// Esqueleto para el comando PRIVMSG (opcional, puedes completarlo luego)
void Server::handlePrivmsg(Message *msg, int sender_fd)
{
	// Aquí va la lógica para enviar mensajes privados o a canales
	std::cout << "[PRIVMSG] Comando recibido de fd " << sender_fd << " " << msg << std::endl;
}

// Get user
// Get parameters
// Check the requested nick is valid
// set new nicname on user (will need a setter on User?)
// DONE Check that the Nick does not already exist - can grab from branch
void	Server::handleNick(Message *msg, User *usr)
{
	std::list<std::string>	_params = msg->getParams();
	if (_params.empty())
	{
		// send  ERR_NONICKNAMEGIVEN (431)
		this->_reply(usr->getFD(), 431);
		return ;
	}
	std::string	newNick = _params.front();
	std::cout << "Trying to set nickname to " << newNick << std::endl;
	// NOTE These characters are forbidden from starting the nick
	std::string	notLeading = "#:&123456789";
	std::string forbidden = " \b\n\r";

	if ((newNick.find_first_of(notLeading) == 0) ||
		(newNick.find_first_of(forbidden) != std::string::npos))
	{
		std::cerr << "Bad characters in nickname" << std::endl;
		// send  ERR_ERRONEUSNICKNAME (432)
		this->_reply(usr->getFD(), 432);
	}
	else if (_isNickTaken(newNick, usr->getFD()))
	{
		std::cerr << "Nickname already in use." << std::endl;
		// send ERR_NICKNAMEINUSE (433)
		this->_reply(usr->getFD(), 433);
	}
	else
	{
		std::cout << "setting nickname to " << newNick << std::endl;
		usr->setNick(newNick);
	}
}

// FIXED Protect against empty _params!
void	Server::handleUser(Message *msg, User *usr)
{
	std::list<std::string>	_params = msg->getParams();
	if (_params.size() != 4)
	{
		std::cerr << "Not enough parameters" << std::endl;
		// send  ERR_NEEDMOREPARAMS (461)
		this->_reply(usr->getFD(), 461);
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
}

Server::~Server(void)
{
	// Libera recursos si es necesario
	std::cout << "Server destructor called." << std::endl;
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

// NOTE This works provided there is no \n at the end of the parameters
// DONE Don't touch User if already verfified
void	Server::handlePass(Message *msg, User *usr) const
{
	std::string	_sPass = this->_password;
	std::list<std::string>	_cPass = msg->getParams();
	std::string	_cmd = msg->getCommand();

	if (_cPass.empty())
	{
		// ERR_NEEDMOREPARAMS
		_reply(usr->getFD(), 461);
		return ;
	}
	if (_sPass.compare(_cPass.front()) == 0)
	{
		std::cout << "Password match!" << std::endl;
		if (!(usr->isVerified()))
			usr->switchVerification();
		else	 // ERR_ALREADYREGISTERED
			_reply(usr->getFD(), 462);
		// TODO Server sends some kind of acknowledgment?
	}
	else
	{
		// TODO send ERR back client
		// ERR_PASSWORDMISMATCH
		_reply(usr->getFD(), 464);
		// TODO disconnect them by implementing the ERROR command
	}
}

// Run through the Messages in the _toProcess queue
// Act on them, delete them.
// TODO Make this spin off thread(s) to process the command efficiently
// NOTE How do we make sure that this is non-blocking?
// TODO Need some kind of matching / switch-case logic here (I guess)
// DONE First check that the User is allowed to have the MEssage processed
// ...i.e. if no Pass, that is the only allowed action
// NOTE This function has friend-based access to Message so it can extract the User involved
// DONE Implement the commands needed for registration:
// [X] CAP -- we could do this for a bonus but for just ignoring
// [X] NICK
// [X] USER
// [X] PASS - partly done, make consistent
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
		if (command.compare("PASS") == 0)
			handlePass(do_next, do_next->getOrigin());
		else if (command.compare("CAP") == 0)
			std::cout << "Ignoring capability negotiation request" << std::endl;
		// NOTE Probably KICK & PRIVMSG should change to accept a USER not FD
		else if (do_next->_origin->isVerified())
		{
			if (command.compare("NICK") == 0)
				handleNick(do_next, do_next->getOrigin());
			if (command.compare("USER") == 0)
				handleUser(do_next, do_next->getOrigin());
			else if (command == "KICK")
				handleKick(do_next, do_next->getOrigin()->getFD());
			else if (command == "PRIVMSG")
				handlePrivmsg(do_next, do_next->getOrigin()->getFD());
		}
		// deleting the Message here seems to reduce "still reachable" type leaks
		delete do_next;
 	}
}

// Send a numeric reply in response to a command received
// NOTE that this is more easily called if we import a
// bunch of enums in a header, or similar.
// TODO Decide how to handle the PARAMETERS that some messages need
// ...call in the function? Lookup in another map? Transform in another function?
// NOTE std::to_string() is c++17 onwards, forbidden :'(
// NOTE We don't need to include EAGAIN because it enums the same as EWOULDBLOCK
void	Server::_reply(int send_to, int msg) const
{
	std::stringstream strm;
	strm << msg;
	std::string msg_as_str = strm.str();
	// const here is to avoid -fpermissive compiler warning
	const char*		msg_buf = msg_as_str.c_str();
	size_t		str_len = msg_as_str.length();

	std::cout << "Sending:" << msg_buf << std::endl;
	if (send(send_to, msg_buf, str_len, MSG_DONTWAIT) == -1)
	{
		// check error number and handle it
		switch (errno)
		{
			case EWOULDBLOCK:
				std::cerr << "Would block, split message or drop it" << std::endl;
				break ;
			default:
				std::cerr << "Dunno, something else went wrong" << std::endl;
		}
	}
	else
	{
		std::cout << "Server reply message sent OK" << std::endl;
	}
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
