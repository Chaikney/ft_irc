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

// FIXED? Away sends a NEEDMOREPARAMS error
Away::Away(Server* srv, Message &seed) : ACommand(srv, seed, 0, 1)
{
	std::cerr << "Bare Away constructor called, hope that is not a problem..." << std::endl;
}

Away::~Away(void) {}

// No, or empty parameter = NOT away
// otherwise: going away, broadcast message
// TODO Handle going-away message (e.g. broadcast to channel)
// DONE? *Store* going away message for use in RPL_AWAY
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
		std::string	farewell_note = _msg.getParams().back();
		usr->setAway(true);
		usr->setAwayMsg(farewell_note);
		this->_responses.push(Message::_reply(_msg, RPL_NOWAWAY));
		// TODO We need to get *all* the User's channels
		// TODO If the user is Invisible, do we say anything?
//		this->_responses.push(Message::_channelMessage(*msg, chan));
	}
}
