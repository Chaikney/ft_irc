#include "ACommand.hpp"
#include "Away.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

Away::Away(Server* srv, Message &seed) : ACommand(srv, seed, 0, 1)
{
	std::cerr << "Bare Away constructor called, hope that is not a problem..." << std::endl;
}

Away::~Away(void) {}

// No, or empty parameter = NOT away
// otherwise: going away, set message and status
// NOTE Not going to handle going-away message broadcast to channel
void	Away::executeCmd(void)
{
	User*	usr = _msg.getOrigin();
	if (usr)
	{
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
		}
	}
}
