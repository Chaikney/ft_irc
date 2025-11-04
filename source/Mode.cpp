#include "ACommand.hpp"
#include "Mode.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// MODE target (modestring) (mode parameters)
Mode::Mode(Server* srv, Message &seed) : ACommand(srv, seed, 1, 3)
{
	std::cerr << "Mode constructor called..." << std::endl;
}

Mode::~Mode(void) {}

// MODE command to deal with a User
// NOTE User modes don't need the modearg, are all type D
// TODO Notify on changed modes, will need to call User::+setModeLetter and act on the return
// (This is partly done but will be unreliable)
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
		if (target_user->setMode(modestring))
		{
			// Send a MODE change notification to the user
			this->_responses.push(Message::_reply(*msg, RPL_UMODEIS, msg->getOrigin()));
			std::cout << "User mode has changed" << std::endl;	// HACK debugging
		}
	}
}

// RPL_BANLIST (367)
//   "<client> <channel> <mask> [<who> <set-ts>]"
// RPL_ENDOFBANLIST (368)
//  "<client> <channel> :End of channel ban list"
//  FIXME These might not be the correct _reply calls to use
void	Mode::_sendBanList(Channel* chan)
{
	std::set<std::string>	banned = chan->getBannedNicks();
	std::set<std::string>::const_iterator it = banned.begin();
	while (it != banned.end())
	{
		Message* reply;
		reply = Message::_reply(_msg, RPL_BANLIST, chan);
		reply->addParams(*it);
		this->_responses.push(reply);
		it++;
	}
	this->_responses.push(Message::_reply(_msg, RPL_ENDOFBANLIST, chan));
}

// MODE command to deal with a Channel
// TODO If channel mode changes, broadcast the change to channel
// NOTE Here, params has popped off the first (target), it is not like msg->getParams()
// TODO How to handle +b ? (request for a ban list)? In channel i suppose
// When the server is done processing the modes,
// a MODE command is sent to all members of the channel containing the mode changes.
// Servers MAY choose to hide sensitive information when sending the mode changes.
// FIXED Password mode is not set, does not send parameter
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
	params.pop_front();	// discard target, we already have it
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
	// FIXME So how then do we treat +b for banlist? Could still be privileged information.
	// NOTE Ban is not required in the subject, think of as bonus
	if (!channel->isOperator(usr))
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
	if (channel->setMode(modestring, modearg))
	{
		// TODO Notify all in channel of changed mode (target->getModeString() and what else?)
		std::cout << "Channel modes for " << target << "have been changed" << std::endl;
	}
}

// Command: MODE
// Parameters: <target> [<modestring> [<mode arguments>...]]
// TODO Handle modestring-less commands with a reply
// - param 1 = target, either Nick or Channel
// - param 2 = optional modestring
// - param 3 = optional mode arguments
// First we decide if we are targetting a channel or aa user, and direct appropriately
// After changes are made, we have to notify them - individually or together?
// NOTE Workaround for Hexchat which sends a blank final parameter.
// FIXME Sends not enough params for MODE #channel +b which is absolutely fine
// TODO Return Ban list on MODE #channel +b
// There are four categories of channel modes, defined as follows:
// Type A: Modes that add or remove an address to or from a list.
// ...These modes MUST always have a parameter when sent from the server to a client.
// ....A client MAY issue this type of mode without an argument to obtain the current contents of the list. The numerics used to retrieve contents of Type A modes depends on the specific mode. Also see the EXTBAN parameter.
// Type B: Modes that change a setting on a channel.
// ...These modes MUST always have a parameter.
// Type C: Modes that change a setting on a channel.
// ...These modes MUST have a parameter when being set, and MUST NOT have a parameter when being unset.
// Type D: Modes that change a setting on a channel.
// ...These modes MUST NOT have a parameter.
void	Mode::executeCmd(void)
{
    std::list<std::string> params = _msg.getParams();
	std::string	target;
	if (!params.empty())
		target = params.front();
	else
		throw (std::logic_error ("MODE command with empty parameters"));
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
		User::normaliseNick(&target);
		this->_userMode(&_msg, _msg.getOrigin(), target);
		return ;
	}
}
