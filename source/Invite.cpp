#include "ACommand.hpp"
#include "Invite.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// Invite::Invite(void)
// {
// 	std::cerr << "Bare Invite constructor should not be called" << std::endl;
// }

Invite::Invite(Server* srv, Message &seed) : ACommand(srv, seed, 2, 2)
{
	std::cerr << "Bare Invite constructor called, hope that is not a problem..." << std::endl;
}

Invite::~Invite(void) {}

// INVITE <nick> <channel>
// The target channel SHOULD exist (at least one user is on it).
// Otherwise, the server SHOULD reject the command with the ERR_NOSUCHCHANNEL numeric.
// Only members of the channel are allowed to invite other users.
// Otherwise, the server MUST reject the command with the ERR_NOTONCHANNEL numeric.
// Servers MAY reject the command with the ERR_CHANOPRIVSNEEDED numeric.
// In particular, they SHOULD reject it when the channel has invite-only mode set, and the user is not a channel operator.
// If the user is already on the target channel,
// the server MUST reject the command with the ERR_USERONCHANNEL numeric.
// When the invite is successful,
// the server MUST send a RPL_INVITING numeric to the command issuer,
// and an INVITE message, with the issuer as <source>, to the target user.
// Other channel members SHOULD NOT be notified.
// TODO Invite notification through a standard method (that works)
// TODO Add invited NICK to two of the responses (RPL_INVITING and ERR_USERONCHANNEL)
void	Invite::executeCmd(void)
{
    std::list<std::string> params = _msg.getParams();
    if (params.size() < 2) return ;
	User*	usr = _msg.getOrigin();
    std::string nick = params.front(); params.pop_front();
    std::string chan = params.front();
    Channel *channel = this->_srv->_findChannel(chan);
    if (!channel)
    {
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
        return ;
    }
	if (!channel->isMember(usr))
    {
		this->_responses.push(Message::_reply(_msg, ERR_NOTONCHANNEL));
        return ;
    }
    if (!channel->isOperator(usr))
    {
		this->_responses.push(Message::_reply(_msg, ERR_CHANOPRIVSNEEDED, channel));
        return ;
    }
    channel->addInvite(nick);
    User *target = this->_srv->_findUserByNick(nick);
	if ((target) && channel->isMember(target))
    {
		this->_responses.push(Message::_reply(_msg, ERR_USERONCHANNEL, channel));
        return ;
    }
	else
	{
		this->_responses.push(Message::_reply(_msg, RPL_INVITING, channel));
		// TODO Use standard mechanism to send the message
		if (target)
		{
			std::cerr << "INVITE sending not implemented yet" << std::endl;
		}
	}
}
