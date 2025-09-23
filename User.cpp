#include "User.hpp"

// FIXME Parameters needed to create a User from nothing?
// Start with blank names, password FALSE
// ...but the connection information?
// The first thing we see is a file descriptor I think. What else in the socket
User::User(void) : _nick(""), _uname(""), _rname(""),
									 _gavepass(false)
{}

// User Destructor should handle itself unless we add more things
User::~User(void)
{}

// Give [whatever], get back a User object to use wherever
// TODO This has to take the info required for a User to be created
User	*User::makeUser(void)
{
	User	*usr = new User();
	return (usr);
}

User::User(const User &original): _nick(original._nick), _uname(original._uname),
										   _rname(original._rname), _gavepass(original._gavepass)
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
