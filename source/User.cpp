#include "User.hpp"
#include <arpa/inet.h>	// inet_ntop() ("network-to-printable")
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>	// used in setMode()
#include <set>	// store the user's set of memberships

// Start with blank names, password FALSE
// ...but the connection information?
// The first thing we see is a file descriptor I think. What else in the socket
User::User(void) : _fd(-1), _nick(""), _uname(""), _rname(""),
				   _gavepass(false), _address(), _host(), last_seen(), _isAway(false), _isServerOp(false),
				   _isInvisible(false), _memberships()
{
	std::cerr << "Cannot create User instance without a socket fd" << std::endl;
}

// TODO Add debug info about the address
// TODO Catch more possible problems with the creation
// NOTE Cannot get hostname so taking the IP addr
User::User(int fd) : _fd(fd), _nick(""), _uname(""), _rname(""),
					 _gavepass(false), _address(), _host(), last_seen(), _isAway(false), _isServerOp(false),
					 _isInvisible(false), _memberships()
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
	time(&last_seen);
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
								  _address(original._address), _host(original._host),
								  last_seen(original.last_seen), _isAway(original._isAway), _isServerOp(original._isServerOp),
								  _isInvisible(original._isInvisible), _memberships(original._memberships)
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

// Add a channel to the user's set of memberships
void	User::addChannel(Channel* channel)
{
	if (channel)
		this->_memberships.erase(channel);
}

// Remove a channel from the user's set of memberships
void	User::removeChannel(Channel* channel)
{
	// Simple implementation - just remove channel name
	// In a full implementation, this would remove from a set of channels
	if (channel)
		this->_memberships.erase(channel);
}

// Method to be triggered by Server when we receive a message from the user
// This way, we can set away or disconnect clients that we never hear from.
void	User::updateTime(void)
{
	this->last_seen = time(0);
}

// NOTE Could be overloaded with a Channel to shape the info
// <flags> contains the following characters, in this order:
// Away status: the letter H ('H', 0x48) to indicate that the user is here,
// or the letter G ('G', 0x47) to indicate that the user is gone.
// Optionally, a literal asterisk character ('*', 0x2A) to indicate that
// the user is a server operator.
// Optionally, the highest channel membership prefix that the client has in <channel>, if the client has one.
// Optionally, one or more user mode characters and other arbitrary server-specific flags.
// TODO Should this handle isInvisible?
std::string	User::getFlags(void) const
{
	std::string	flags;
	if (this->_isAway)
		flags.append("G");
	else
		flags.append("H");
	if (this->_isServerOp)
		flags.append("*");
	return (flags);
}

// Returns a list of parameters for use in the WHOREPLY command
// https://modern.ircdocs.horse/#rplwhoreply-352
// NOTE That the caller should use them with other pieces to make a full reply
// TODO Check whether this should return a pointer or reference instead
std::list<std::string>	User::getWhoReply(void) const
{
	std::list<std::string>	params;
	params.push_back("*");	// NOTE With Channel, this is channel name
	params.push_back(this->getUser());
	params.push_back(this->getHost());
//	params.push_back(SERVER);	// FIXME this is not available
	params.push_back("ft_irc");	// HACK hardcoded to test client behaviour
	params.push_back(this->getNick());
	params.push_back(this->getFlags());
	params.push_back("1");
	params.push_back(this->getReal());
	return (params);
}

// Slightly different from the above, sadly
// https://modern.ircdocs.horse/#rplwhoisuser-311
std::list<std::string>	User::getWhoIs(void) const
{
	std::list<std::string>	params;
	params.push_back(this->getNick());
	params.push_back(this->getUser());
	params.push_back(this->getHost());
	params.push_back("*");
	params.push_back(this->getReal());
	return (params);
}

// Return the User representation for use in the RPL_USERHOST list
// TODO getHost representation, should it be like user@hostname ?
std::string	User::getUserHostMsg(void) const
{
	std::string	userHost;
	userHost.append(this->getNick());
	if (this->_isServerOp)
		userHost.append("isop");
	userHost.append("=");
	if (this->_isAway)
		userHost.append("-");
	else
		userHost.append("+");
	userHost.append(this->getHost());
	return (userHost);
}

void	User::setAway(bool are)
{
	this->_isAway = are;
}

// Read a modestring into pieces and pass to the active method.
// NOTE Ignoring parameters (unlike Channel::SetMode) because they aren't needed
// (see https://defs.ircdocs.horse/defs/usermodes.html - only 2 have parameters, ignorable)
bool	User::setMode(std::string modestring)
{
	bool adding = true;
	std::stringstream	strm(modestring);
	char	c = 0;

	while (strm)	// or whatever
	{
		c = strm.get();
		if (c == '+')
			adding = true;
		else if (c ==  '-')
			adding = false;
		else
			_setModeLetter(c, adding, "");	// HACK ignores parameters (because I don't understand them)
	}
	return true;	// just because
}

// TODO Support supported user modes:
// +oO
// +i (lter)
// NOTE OPER command is needed to support isOp properly
bool User::_setModeLetter(char mode, bool add, const std::string &param)
{
	(void) param;	// HACK because no modes use this (yet?)
	switch (mode)
	{
		case 'o':
			this->_isServerOp = add;
			return (true);	// TODO Probably should check for ability to take this away...
		case 'O':
			this->_isServerOp = add;
			return (true);	// TODO Probably should check for ability to take this away...
		case 'i':
			this->_isInvisible = add;
			return (true);
	default:
		return false;
	}
}

// Return the User's active modes, as used in RPL_UMODEIS
// NOTE In future could add +w WALLOPS
// Status of +o Network Operator is ambiguous - we have no network
// TODO Consider making User::getModes private - who else uses it?
std::string	User::getModes(void) const
{
	std::string	modes;
	if (this->_isInvisible)
		modes.append("i");
	if (this->_isServerOp)
		modes.append("O");
	if (this->isRegistered())
		modes.append("r");
	// Finally, if the return is not blank, it starts with a +
	if (!modes.empty())
		modes = "+" + modes;
	return (modes);
}

bool	User::isAway(void) const
{
	return (this->_isAway);
}

std::set<Channel *>	User::getMemberships(void) const
{
	return (this->_memberships);
}

// Remove any valid but confusing prefix characters from the Nick
// return true if we can work with the string after modifications
// TODO Confirm all the possible prefix characters
// TODO Make sure no spaces in the parameter - this should not happen but seems to...
bool	User::normaliseNick(std::string *nick)
{
	if (nick->empty())
		return (false);
	if (nick->find(' ') != std::string::npos)
		return (false);
	while (nick->find_first_of("@:") == 0)
		nick->erase(0, 1);
	std::cout << "Nick targeted will be:" << *nick << std::endl;	// HACK for debugging
	return (true);
}
