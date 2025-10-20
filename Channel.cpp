#include "Channel.hpp"
#include "User.hpp"
#include <iostream>
#include <cstdlib>
#include <set>
#include <sstream>

Channel::Channel(void) : _name(""), _creationTime(time(0)), _topic(""), _topicTime(time(0)), _topicSetBy("Server"), _members(), _operators(), _invitedNicks(),
						_topicProtected(false), _noExtMsg(true), _inviteOnly(false), _password(""),
						_userLimit(0), _banList(), _exceptionList(), _inviteList()
{
}

Channel::Channel(const std::string &name) : _name(name),_creationTime(time(0)),  _topic(""),_topicTime(time(0)), _topicSetBy("Server"),  _members(), _operators(),
											_invitedNicks(), _topicProtected(false), _noExtMsg(true), _inviteOnly(false),
											_password(""), _userLimit(0), _banList(), _exceptionList(), _inviteList()
{
}

Channel::Channel(const Channel &other) : _name(other._name),_creationTime(other._creationTime),  _topic(other._topic),_topicTime(time(0)), _topicSetBy("Server"),  _members(other._members),
										_operators(other._operators), _invitedNicks(other._invitedNicks),
										_topicProtected(other._topicProtected), _noExtMsg(other._noExtMsg), _inviteOnly(other._inviteOnly),
										_password(other._password), _userLimit(other._userLimit),
										_banList(other._banList), _exceptionList(other._exceptionList),
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
		_banList = other._banList;
		_exceptionList = other._exceptionList;
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

const std::set<int>& Channel::getOperators(void) const
{
	return _operators;
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
	return _inviteOnly;
}

bool Channel::hasPassword(void) const
{
	return !_password.empty();
}

const std::string& Channel::getPassword(void) const
{
	return _password;
}

int Channel::getUserLimit(void) const
{
	return _userLimit;
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
bool Channel::addMember(User *usr)
{
	if (_members.find(usr) != _members.end())
		return false; // Already a member

	_members.insert(usr);

	// First member becomes operator
	if (_members.size() == 1)
		_operators.insert(usr->getFD());

	return true;
}

bool Channel::removeMember(User *usr)
{
	if (_members.find(usr) == _members.end())
		return false; // Not a member

	_members.erase(usr);
// TODO Restore the Operator erase piece here
//	_operators.erase(usr); // Remove operator status if they had it
	return true;
}

bool Channel::isMember(User *usr) const
{
	return (_members.find(usr) != _members.end());
}

bool Channel::isOperator(int userFd) const
{
	return _operators.find(userFd) != _operators.end();
}

// TODO Needs updated
bool Channel::addOperator(User *usr)
{
	if (_members.find(usr) == _members.end())
		return false; // Must be a member first

	// TODO Update this later.
	_operators.insert(usr->getFD());
	return true;
}

bool Channel::removeOperator(int userFd)
{
	if (_operators.find(userFd) == _operators.end())
		return false; // Not an operator

	_operators.erase(userFd);
	return true;
}

// Invite management
bool Channel::addInvite(const std::string &nick)
{
	_invitedNicks.insert(nick);
	return true;
}

bool Channel::removeInvite(const std::string &nick)
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

// Ban management
bool Channel::addBan(const std::string &mask)
{
	_banList.insert(mask);
	return true;
}

bool Channel::removeBan(const std::string &mask)
{
	if (_banList.find(mask) == _banList.end())
		return false;

	_banList.erase(mask);
	return true;
}

bool Channel::isBanned(const std::string &mask) const
{
	return _banList.find(mask) != _banList.end();
}

// Mode management

// Return the channel's mode string
// NOTE We support here tkil -
// how do we handle reporting the ones with parametrers, i.e. limit?
std::string Channel::getModeString(void) const
{
	std::string modes = "+";

	if (_topicProtected)
		modes += "t";
	if (_inviteOnly)
		modes += "i";
	if (hasPassword())
		modes += "k";
	if (_userLimit > 0)
		modes += "l";

	return modes;
}

// This should ingest like C++ not C
// get a value for adding, flag and param
// pass that to the other setMode function
// FIXME This wouldn't carry a parameter, as the string would be lost?
// NOTE This could be const, as the changes happen in the other setMode(?)
bool	Channel::setMode(std::string modestring)
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
			setMode(c, adding, "");	// HACK ignores parameters (because I don't understand them)
	}
	return true;	// just because
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
			_topicProtected = add;
			return true;
		case 'i':
			_inviteOnly = add;
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
				_userLimit = atoi(param.c_str());
			}
			else
				_userLimit = 0;
			return true;
		case 'n':
			this->_noExtMsg = add;
			return (true);
		default:
			return false;
	}
}

// Utility
bool Channel::isEmpty(void) const
{
	return _members.empty();
}

void Channel::clear(void)
{
	_members.clear();
	_operators.clear();
	_invitedNicks.clear();
	_banList.clear();
	_exceptionList.clear();
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
		if (this->isOperator(member->getFD()))
			name += "@";
		name += member->getNick();
		params.push_back(name);
		name.clear();
		it++;
	}
	return (params);
}

User*		Channel::findMemberByNick(std::string target) const
{
	for (std::set<User*>::const_iterator it = this->_members.begin(); it != this->_members.end(); ++it)
    {
        if ((*it)->getNick() == target)
            return (*it);
    }
    return NULL;
}
