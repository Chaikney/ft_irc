#include "User.hpp"
#include <unistd.h>

// FIXME Parameters needed to create a User from nothing?
// Start with blank names, password FALSE
// ...but the connection information?
// The first thing we see is a file descriptor I think. What else in the socket
User::User(void) : _nick(""), _uname(""), _rname(""),
									 _gavepass(false), _address()
{}

// TODO Add debug info about the address
User::User(int fd) : _nick(""), _uname(""), _rname(""),
									 _gavepass(false), _address()
{
	socklen_t	addr_size;	// I only made this for getsockname and I guess error checking
	// Get more info about the socket
	// TODO Perhaps this should be part of User creation and storage?
	if (getsockname (fd, (struct sockaddr *) &_address, &addr_size) == -1)
	{
		close (fd);
		throw std::runtime_error("Failed to get client info");
	}
	std::cout << "User created for fd:" << fd << std::endl;
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

User::User(const User &original): _nick(original._nick), _uname(original._uname),
										   _rname(original._rname), _gavepass(original._gavepass),
								  _address(original._address)
{}

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
