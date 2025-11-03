#include "KickCmd.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"
#include <stdexcept>

KickCmd::KickCmd(Server *srv, Message &msg) : ACommand(srv, msg) {}

KickCmd::~KickCmd() {}

// Take the suppposed channel name, perform normalisation and locate the Channel on the server
// Returns the channel (could be null)
// While checking, enqueues relevant error messages
// TODO KICKing User must be in chanel and an operator, right?
Channel*	KickCmd::_checkChannel(std::string str)
{
	Channel*	chan = 0;
    if (Channel::normaliseChanName(&str) == false)
        _responses.push(Message::_reply(_msg, ERR_BADCHANMASK));
	else
	{
		chan = _srv->_findChannel(str);
		if (!chan)
			_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
	}
	return (chan);
}

// Check that the given target NICK is an existing user
User*	KickCmd::_checkUser(std::string nick)
{
	User *target = _srv->_findUserByNick(nick);
    if (!target)
		_responses.push(Message::_reply(_msg, ERR_NOSUCHNICK));
	return (target);
}

// TODO target Nick has to be in the ERR_ message...
// "<client> <nick> <channel> :They aren't on that channel"
bool	KickCmd::_checkCombo(User *target, Channel *chan, User *usr)
{
	if (!chan->isMember(target))
	{
		_responses.push(Message::_reply(_msg, ERR_USERNOTINCHANNEL));
		return (false);
	}
	else if (!chan->isOperator(usr))
	{
		_responses.push(Message::_reply(_msg, ERR_CHANOPRIVSNEEDED, chan));
		return (false);
	}
	return (true);
}

// KICK <channel> <user> [<comment>]
// TODO Adapt to handle multiple Users getting kicked from the one channel
// (Although: "Servers MAY limit the number of target users per KICK command via the TARGMAX parameter
// of RPL_ISUPPORT, and silently drop targets if the number of targets exceeds
// the limit.)" - still implies only reading to the first comma
// FIXME Nothing happens!
void KickCmd::executeCmd(void)
{
//    User *usr = _msg.getOrigin();
	// if (!usr)
	// 	throw(std::logic_error("User who sent KICK command does not exist"));
    std::list<std::string> params = _msg.getParams();
    std::string chan = params.front();
	params.pop_front();
    std::string nick = params.front();
	params.pop_front();
    std::string reason = "";
	if (!params.empty())
        reason = params.front();
	if (reason.empty())
		reason = "Kicked";
	Channel*	channel = _checkChannel(chan);
	if (!channel)
	{
		std::cerr << "failed to find channel" << std::endl;
		return;
	}
	//  Loop here over every given NICK
	User*	target =_checkUser(nick);
	if (target)
	{
		if (this->_checkCombo(target, channel, _msg.getOrigin()))
		{
			if (channel->removeMember(target))
				std::cout << target->getNick() << " removed from " << channel->getName() << std::endl;
			else
				std::cerr << target->getNick() << " NOT removed from " << channel->getName() << std::endl;
			(target->removeChannel(channel));
//				std::cout << target->getNick() << " record removed from " << channel->getName() << std::endl;
			// FIXME direct notification doesn't seem to get sent
			Message*	chanNotice = Message::_channelMessage(_msg, channel);
			chanNotice->addParams(target->getNick());
			chanNotice->addParams(reason);
			_responses.push(chanNotice);
			Message *direct = Message::_replyNonNumeric(_msg, channel);
			direct->addParams(target->getNick());
			direct->addParams(reason);
			_responses.push(direct);
		}
		else
			std::cerr << "not sending anythin" << std::endl;
	}
	else
	{
		std::cerr << "failed to find user" << std::endl;
	}
}
