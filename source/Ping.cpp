#include "ACommand.hpp"
#include "Ping.hpp"

# include <iostream>

// Ping::Ping(void)
// {
// 	std::cerr << "Bare Ping constructor should not be called" << std::endl;
// }

Ping::Ping(Server* srv, Message &seed) : ACommand(srv, seed, 0, 1)
{
	std::cerr << "Bare Ping constructor called, hope that is not a problem..." << std::endl;
}

Ping::~Ping(void) {}

// Client says PING <token> then we return PONG <token>
// As PONG only comes back in reponse to PING,
// if we don't *send* a PING then there's no need to handle PONG
// TODO Use usr to update a "last seen" value for AWAY, autodisconnects, etc
// NOTE most of this could be replaced in-loop with the ACommand* =  and enqueue / processing...
void	Ping::executeCmd(void)
{
	Message	msg = this->_msg;
    std::list<std::string> params = this->_msg.getParams();
    std::string origin = params.front();
    if (origin.empty())
    {
        this->_responses.push(Message::_reply(msg, ERR_NOORIGIN));
        return ;
    }
	// HACK send NULL for lack of overload, dangerous!
	// TODO This pushes too much logic to the Message class;
	//  better get a blank message then add to it here.
	this->_responses.push(Message::_replyNonNumeric(msg));
}
