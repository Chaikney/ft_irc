#include "ACommand.hpp"
#include "Away.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// AwaAwayite(void)
// {
// 	std::cerr << "Bare Away constructor should not be called" << std::endl;
// }

Away::Away(Server* srv, Message &seed) : ACommand(srv, seed, 2, 2)
{
	std::cerr << "Bare Away constructor called, hope that is not a problem..." << std::endl;
}

Away::~Away(void) {}

// No, or empty parameter = NOT away
// otherwise: going away, broadcast message
// TODO Handle going-away message (e.g. broadcast to channel)
void	Away::executeCmd(void)
{
	User*	usr = _msg.getOrigin();
	if (_msg.getParams().empty() && (usr->isAway()))
		{
			usr->setAway(false);
			this->_responses.push(Message::_reply(_msg, RPL_UNAWAY));
		}
	else if (!usr->isAway())
	{
		usr->setAway(true);
		this->_responses.push(Message::_reply(_msg, RPL_NOWAWAY));
		// TODO We need to get *all* the User's channels
		// TODO If the user is Invisible, do we say anything?
//		this->_responses.push(Message::_channelMessage(*msg, chan));
	}
}
