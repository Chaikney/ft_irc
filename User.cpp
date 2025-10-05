#include "User.hpp"
#include <arpa/inet.h>	// inet_ntop() ("network-to-printable")
#include <netinet/in.h>
#include <unistd.h>

// Start with blank names, password FALSE
// ...but the connection information?
// The first thing we see is a file descriptor I think. What else in the socket
User::User(void) : _fd(-1), _nick(""), _uname(""), _rname(""),
				   _gavepass(false), _address(), _host()
{
	std::cerr << "Cannot create User instance without a socket fd" << std::endl;
}

// TODO Add debug info about the address
// TODO Catch more possible problems with the creation
// NOTE Cannot get hostname so taking the IP addr
User::User(int fd) : _fd(fd), _nick(""), _uname(""), _rname(""),
					 _gavepass(false), _address(), _host()
{
	socklen_t	addr_size = INET_ADDRSTRLEN;	// I only made this for getsockname and I guess error checking
	char	ip_addr[INET_ADDRSTRLEN];
	// Get more info about the socket
	if (getsockname (fd, (struct sockaddr *) &_address, &addr_size) == -1)
	{
		close (fd);
		throw std::runtime_error("Failed to get client info");
	}
	inet_ntop(AF_INET, &_address.sin_addr, ip_addr, INET_ADDRSTRLEN);
	this->_host = ip_addr;
	// HACK debug statements that can be removed
	std::cout << "User created for fd:" << fd << std::endl;
	std::cout << "IP Address: " << this->_host << std::endl;
}


// User Destructor should handle itself unless we add more things
User::~User(void)
{}

// Given a file descriptor, get back a User object to use wherever
// TODO Some checks required - is the fd real, valid, etc.
// TODO This has to take the info required for a User to be created
User	*User::makeUser(int fd)
{
	User	*usr = new User(fd);
	return (usr);
}

User::User(const User &original): _fd(original._fd), _nick(original._nick), _uname(original._uname),
										   _rname(original._rname), _gavepass(original._gavepass),
								  _address(original._address), _host(original._host)
{}

int	User::getFD() const
{
	return(this->_fd);
}

std::string	User::getNick() const
{
	return(this->_nick);
}

std::string	User::getUser() const
{
	return(this->_uname);
}

std::string	User::getReal() const
{
	return(this->_rname);
}

bool	User::isVerified() const
{
	return(this->_gavepass);
}

// FIXME Location of a segfault from Server Messages in the processQueue
bool	User::isRegistered() const
{
    return (this->_gavepass && !this->_nick.empty() && !this->_uname.empty());
}

sockaddr_in	User::getAddress() const
{
	return(this->_address);
}

std::string	User::getHost() const
{
	return(this->_host);
}

// Made this as a toggle so it can be used to de-verify users,
// should we want to
void	User::switchVerification()
{
	if (this->isVerified())
		this->_gavepass = false;
	else
		this->_gavepass = true;
}

// NOTE This assumes that checking has already happened
void	User::setNick(std::string nick)
{
	this->_nick = nick;
}

// NOTE This assumes that checking has already happened
void	User::setUser(std::string user)
{
	this->_uname = user;
}

// NOTE This assumes that checking has already happened
void	User::setReal(std::string realname)
{
	this->_rname = realname;
}

// FIXME Implement or remove this function
void	User::addChannel(const std::string &channel)
{
	// Simple implementation - just store channel name
	// In a full implementation, this would add to a set of channels
	(void)channel;
	std::cerr << "User::addChannel is not implemented yet" << std::endl;
}

// FIXME Implement or remove this function
void	User::removeChannel(const std::string &channel)
{
	// Simple implementation - just remove channel name
	// In a full implementation, this would remove from a set of channels
	(void)channel;
	std::cerr << "User::removeChannel is not implemented yet" << std::endl;
}
