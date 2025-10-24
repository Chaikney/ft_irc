#include "ACommand.hpp"
#include "Mode.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// Mode::Mode(void)
// {
// 	std::cerr << "Bare Mode constructor should not be called" << std::endl;
// }
// MODE command to deal with a User
// NOTE User modes don't need the modearg
// TODO Notify on changed modes
// FIXME dont need all these to be passed...
void	Mode::_userMode(Message *msg, User *usr, std::string target)
{
	User* target_user = this->_srv->_findUserByNick(target);
	if (!target_user)
	{
		this->_responses.push(Message::_reply(*msg, ERR_NOSUCHNICK));
		return ;
	}
	if (*usr != *target_user)
	{
		this->_responses.push(Message::_reply(*msg, ERR_USERSDONTMATCH));
		return;
	}
	std::list<std::string> params = msg->getParams();
	params.pop_front();	// that was the target
	if (params.empty() || (params.back() == ""))
	{
		this->_responses.push(Message::_reply(*msg, RPL_UMODEIS, msg->getOrigin()));
	}
	else
	{
		std::string	modestring = params.front();
		target_user->setMode(modestring);
	}
}

// MODE command to deal with a Channel
// TODO If channel mode changes, broadcast the change to channel
// NOTE Here, params has popped off the first (target), it is not like msg->getParams()
// TODO How to handle +b ? (request for a ban list)? In channel i suppose
// FIXME dont need all these to be passed...
void	Mode::_channelMode(Message *msg, User *usr, std::string target)
{
	std::list<std::string>	params = _msg.getParams();
	Channel *channel = this->_srv->_findChannel(target);
	if (!channel)
	{
		// TODO How do we include the "wrong" name here? add target somehow
		this->_responses.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
		return ;
	}
	params.pop_front();	// discard target
	// NOTE 2nd check to work around Hexchat sending final blank parameter
	if (params.empty() || (params.back() == ""))
	{
		// We only got a channel so we list the modes and return
		this->_responses.push(Message::_reply(*msg, RPL_CHANNELMODEIS, channel));
		this->_responses.push(Message::_reply(*msg, RPL_CREATIONTIME, channel));
		return ;
	}
	// NOTE Below here only if more than 1 param was given
	// NOTE No privileges needed to get a listing, but from here we change things
	if (!channel->isOperator(usr->getFD()))
	{
		this->_responses.push(Message::_reply(*msg, ERR_CHANOPRIVSNEEDED, channel));
		return ;
	}
	// get the modestring from the second parameter
	// and any mode parameters from the third
	std::string	modestring = params.front();
	std::string	modearg;
	params.pop_front();
	if (params.empty())
		modearg = "";
	else
		modearg = params.front();
	channel->setMode(modestring, modearg);
}

Mode::Mode(Server* srv, Message &seed) : ACommand(srv, seed, 2, 2)
{
	std::cerr << "Bare Mode constructor called, hope that is not a problem..." << std::endl;
}

Mode::~Mode(void) {}

// Command: MODE
// Parameters: <target> [<modestring> [<mode arguments>...]]
// TODO Handle modestring-less commands with a reply
// - param 1 = target, either Nick or Channel
// - param 2 = optional modestring
// - param 3 = optional mode arguments
// First we decide if we are targetting a channel or aa user, and direct appropriately
// After changes are made, we have to notify them - individually or together?
// NOTE Workaround for Hexchat which sends a blank final parameter.
void	Mode::executeCmd(void)
{
    std::list<std::string> params = _msg.getParams();
	if (params.empty())
	{
        this->_responses.push(Message::_reply(_msg, ERR_NEEDMOREPARAMS));
        return ;
	}
    std::string target = params.front();
//	params.pop_front();	// NOTE This causes a crash below if we remove the final parameter...
	std::cout << "Directing mode for:" << target << std::endl;	// HACK debug statement
	if (target.find_first_of("#&") == 0)// Do MODE as Channel
	{
		std::cout << "Choosing channel" << std::endl;	// HACK debug statement
		this->_channelMode(&_msg, _msg.getOrigin(), target);
		return;
	}
	else // treat target as NICK
	{
		std::cout << "Choosing user" << std::endl;	// HACK debug statement
		this->_userMode(&_msg, _msg.getOrigin(), target);
		return ;
	}
}
