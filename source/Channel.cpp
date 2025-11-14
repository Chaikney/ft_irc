#include "Channel.hpp"
#include "User.hpp"
#include <iostream>
#include <cstdlib>
#include <set>
#include <sstream>

Channel::Channel(void) : _name(""), _creationTime(time(0)), _topic(""), _topicTime(time(0)), _topicSetBy("Server"), _members(), _operators(), _invitedNicks(),
						_topicProtected(false), _noExtMsg(true), _inviteOnly(false), _password(""),
						_userLimit(0), _inviteList()
{
}

Channel::Channel(const std::string &name) : _name(name),_creationTime(time(0)),  _topic(""),_topicTime(time(0)), _topicSetBy("Server"),  _members(), _operators(),
											_invitedNicks(), _topicProtected(false), _noExtMsg(true), _inviteOnly(false),
											_password(""), _userLimit(0), _inviteList()
{
}

Channel::Channel(const Channel &other) : _name(other._name),_creationTime(other._creationTime),  _topic(other._topic),_topicTime(time(0)), _topicSetBy("Server"),  _members(other._members),
										_operators(other._operators), _invitedNicks(other._invitedNicks),
										_topicProtected(other._topicProtected), _noExtMsg(other._noExtMsg), _inviteOnly(other._inviteOnly),
										_password(other._password), _userLimit(other._userLimit),
										_inviteList(other._inviteList)
{
}

Channel::~Channel(void)
{
}

Channel& Channel::operator=(const Channel &other)
{
	if (this != &other)
	{
		_name = other._name;
		_creationTime = other._creationTime;
		_topic = other._topic;
		_topicTime = other._topicTime;
		_topicSetBy = other._topicSetBy;
		_members = other._members;
		_operators = other._operators;
		_invitedNicks = other._invitedNicks;
		_topicProtected = other._topicProtected;
		_noExtMsg = other._noExtMsg;
		_inviteOnly = other._inviteOnly;
		_password = other._password;
		_userLimit = other._userLimit;
		_inviteList = other._inviteList;
	}
	return *this;
}

// Getters
const std::string& Channel::getName(void) const
{
	return _name;
}

const std::string& Channel::getTopic(void) const
{
	return (this->_topic);
}

const std::string& Channel::getTopicSetter(void) const
{
	return (this->_topicSetBy);
}

const std::string Channel::getCreationTime(void) const
{
	std::stringstream	strm;
	strm << this->_creationTime;
	std::string	time_set;
	strm >> time_set;
	return (time_set);
}

const std::string Channel::getTopicTime(void) const
{
	std::stringstream	strm;
	strm << this->_topicTime;
	std::string	time_set;
	strm >> time_set;
	return (time_set);
}

const std::set<User *>& Channel::getMembers(void) const
{
	return (_members);
}

const std::set<User *>& Channel::getOperators(void) const
{
	return (this->_operators);
}

const std::set<std::string>& Channel::getInvitedNicks(void) const
{
	return _invitedNicks;
}

bool Channel::isTopicProtected(void) const
{
	return _topicProtected;
}

bool Channel::isInviteOnly(void) const
{
	return (this->_inviteOnly);
}

bool Channel::hasPassword(void) const
{
	return !_password.empty();
}

const std::string& Channel::_getPassword(void) const
{
	return _password;
}

size_t Channel::getUserLimit(void) const
{
	return _userLimit;
}

std::string Channel::getUserLimitText(void) const
{
	size_t	n = _userLimit;
	std::stringstream	strm;
	strm << n;
	std::string	str;
	str = strm.str();
	return (str);
}

size_t Channel::getMemberCount(void) const
{
	return _members.size();
}

std::string Channel::getMemberCountText(void) const
{
	size_t	n = _members.size();
	std::stringstream	strm;
	strm << n;
	std::string	str;
	str = strm.str();
	return (str);
}

// Setters
void Channel::setTopic(const std::string &topic, const std::string &set_by)
{
	this->_topic = topic;
	this->_topicTime = time(0);
	this->_topicSetBy = set_by;
}

void Channel::setTopicProtected(bool topicProtected)
{
	_topicProtected = topicProtected;
}

void Channel::setInviteOnly(bool inviteOnly)
{
	_inviteOnly = inviteOnly;
}

void Channel::setPassword(const std::string &password)
{
	_password = password;
}

void Channel::clearPassword(void)
{
	_password.clear();
}

void Channel::setUserLimit(int limit)
{
	_userLimit = limit;
}

// Member management
// Add a member to a channel, return TRUE if success.
// - User exists? Not already a member?
// - are they invited?
// - Check and block if user limit would be exceeded
// First member becomes operator
// IDEA Overload this with a version that takes a password
bool Channel::addMember(User *usr)
{
	if (!usr)
		return (false);	// TODO Maybe this should raise an exception? It is a big problem...
	if (_members.find(usr) != _members.end())
		return false; // Already a member
	if (this->isInviteOnly())
	{
		if (!this->isInvited(usr->getNick()))
			return (false);
		else
			this->_removeInvite(usr->getNick());
	}
	if (this->getUserLimit() != 0)
	{
		if (this->getMemberCount() > (this->getUserLimit() - 1))
			return (false);
	}
	_members.insert(usr);
	if (_members.size() == 1)
		this->_operators.insert(usr);
	return (true);
}

// FIXME Is this the place to check whether the channel is empty and should be deleted?
// Confirm that we have a user and that they are in _members
// If so, remove from the 2 places they might be stored.
bool Channel::removeMember(User *usr)
{
	if (!usr)
		return (false);
	if (_members.find(usr) == _members.end())
		return false; // Not a member
	_members.erase(usr);
	_operators.erase(usr); // Remove operator status if they had it
	return true;
}

bool Channel::isMember(User *usr) const
{
	if (!usr)
		return (false);
	else
		return (_members.find(usr) != _members.end());
}

// If the user is an operator, it will be found in the set
bool Channel::isOperator(User *usr) const
{
	return _operators.find(usr) != _operators.end();
}

bool Channel::_addOperator(User *usr)
{
	if (!usr)
		return (false);
	if (_members.find(usr) == _members.end())
		return false; // Must be a member first
	_operators.insert(usr);
	return true;
}

// NOTE Does no-one call this?
bool Channel::_removeOperator(User *usr)
{
	if (this->_operators.find(usr) == _operators.end())
		return false; // Not an operator
	_operators.erase(usr);
	return true;
}

// Invite management
bool Channel::addInvite(const std::string &nick)
{
	_invitedNicks.insert(nick);
	return true;
}

bool Channel::_removeInvite(const std::string &nick)
{
	if (_invitedNicks.find(nick) == _invitedNicks.end())
		return false;

	_invitedNicks.erase(nick);
	return true;
}

bool Channel::isInvited(const std::string &nick) const
{
	return _invitedNicks.find(nick) != _invitedNicks.end();
}

// Return the channel's mode string
// NOTE We support here tkil -
// how do we handle reporting the ones with parametrers, i.e. limit?
std::string Channel::getModeString(void) const
{
	std::string modes;

	if (_topicProtected)
		modes += "t";
	if (_inviteOnly)
		modes += "i";
	if (hasPassword())
		modes += "k";
	if (_userLimit > 0)
	{
		modes += "l ";
		modes += this->getUserLimitText();
	}
	if (!modes.empty())
		modes = "+" + modes;
	return (modes);
}

// This should ingest like C++ not C
// get a value for adding, flag and param
// Identify the final modestring char and only then
// pass param to the other setMode function
// TODO ThIs maybe has to be passed back out to Mode.cpp
// FIXME Can only notify of changes based on the last change made
// HACK Solution to the modestring thing -- alwayts pass it.
// This means we would have no chance of handling multiple mode changes
bool	Channel::setMode(std::string modestring, std::string modearg)
{
	bool adding = true;
	bool	changes = false;
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
		{
			changes = setMode(c, adding, modearg);
		}
	}
	return (changes);
}

// Take a char, boolean and optional string
// Use them to set channel modes per character
// Return true if value changed; false if not
// TODO Should this handle 'o' for operator? (Server or ChanOp??) - is a confusing halfway house
// ...it would add the posting user to the Operators attribute and we cannot access it here...
// DONE Mode +n for no external messages could be added
bool Channel::setMode(char mode, bool add, const std::string &param)
{
	switch (mode)
	{
		case 't':
			if (this->_topicProtected == add)
				return (false);		// no action needed
			else
				this->_topicProtected = add;
			return true;
		case 'i':
			if (this->_inviteOnly == add)
				return (false);		// no action needed
			else
				this->_inviteOnly = add;
			return true;
		case 'k':
			if (add)
			{
				if (param.empty())
					return false;
				_password = param;
			}
			else
				_password.clear();
			return true;
		case 'l':
			if (add)
			{
				if (param.empty())
					return false;
				// NOTE if this is negative then it overflows to a wild value.
				int	new_limit= atoi(param.c_str());
				if (new_limit < 0)
					return (false);
				_userLimit  = (size_t) new_limit;
			}
			else
				_userLimit = 0;
			return true;
		case 'n':
			if (this->_noExtMsg == add)
				return (false); 	// no change
			else
				this->_noExtMsg = add;
			return (true);
		case 'o':
			if (param.empty())
				return (false);
			else
			{
				this->_addOperator(this->_findMemberByNick(param));
				return (true);
			}
		default:
			return false;
	}
}

// Utility
// NOTE May not be used
bool Channel::_isEmpty(void) const
{
	return _members.empty();
}

void Channel::clear(void)
{
	_members.clear();
	_operators.clear();
	_invitedNicks.clear();
	_inviteList.clear();
}

// Return a list of the file descriptors that should receive things
// related to the channel. For use with PRIVMSG, NOTICE, etc
std::list<int>		Channel::getBroadcastFDs(void) const
{
	std::list<int>	targets;
	std::set<User *>	chanMembers = this->getMembers();
	int	fd = 0;
	std::set<User *>::const_iterator it = chanMembers.begin();
	while (it != chanMembers.end())
	{
		User*	heyyou = *it;
		fd = heyyou->getFD();
		targets.push_back(fd);
		it++;
	}
	return (targets);
}

// Removes the sending user from the list of FDs to be sent to
std::list<int>	Channel::getBroadcastFDs(User *notyou) const
{
	std::list<int>	targets = this->getBroadcastFDs();
	targets.remove(notyou->getFD());
	return (targets);
}

// Return parameter list for use in RPL_LIST(322) Message
// "<client> <channel> <client count> :<topic>"
std::list<std::string>	Channel::getListInfo(void) const
{
	std::list<std::string>	params;
	params.push_back(this->getName());
	params.push_back(this->getMemberCountText());
	params.push_back(this->getTopic());
	return (params);
}

// Return a list of channel users formatted for use in RPL_NAMREPLY
// For every member of the channel, build their prefix-NICK pair
// Put it in a list and return it
// TODO Handle secret channels if we ever have them (not just the =)
std::list<std::string>	Channel::getNameReply(void) const
{
	std::list<std::string>	params;
	params.push_back("=");  // = for public channels
	params.push_back(this->getName());
	std::string name;
	std::set<User *> members = this->getMembers();
	std::set<User *>::const_iterator it = members.begin();

	while (it != members.end())
	{
		User* member = *it;
		if (this->isOperator(member))
			name += "@";
		name += member->getNick();
		params.push_back(name);
		name.clear();
		it++;
	}
	return (params);
}

User*		Channel::_findMemberByNick(std::string target) const
{
	for (std::set<User*>::const_iterator it = this->_members.begin(); it != this->_members.end(); ++it)
    {
        if ((*it)->getNick() == target)
            return (*it);
    }
    return NULL;
}

// Helper function, could be moved to Channel or to separate set of functions
// Removes any leading : from the command
// NOTE this should have already happened on Message construction
// Returns TRUE if the string has been turned into a valid channel name
// Returns FALSE if not
// NOTE This modifies the string even if we say we can't do anything with it. Bad!
bool	Channel::normaliseChanName(std::string *chan)
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
	// Channel name too long (IRC limit is 200 characters)
	if (chan->length() > 200)
		return (false);
	// Reject ## or && at the start (invalid channel names)
	if (chan->length() >= 2 && (*chan)[0] == (*chan)[1])
		return (false);
	// Space, BELL and comma are forbidden in Channel names
	if (chan->find_first_of(" ,\a") != std::string::npos)
		return (false);
	return (true);
}

// Return true if the supplied key matches the Channel's password
bool	Channel::checkPassword(std::string const &key) const
{
	if (key.empty())
		return (false);
	return (key.compare(this->_getPassword()) == 0);
}
