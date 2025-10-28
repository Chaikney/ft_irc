#include "ACommand.hpp"
#include "Userhost.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// AwaAwayite(void)
// {
// 	std::cerr << "Bare Userhost constructor should not be called" << std::endl;
// }

Userhost::Userhost(Server* srv, Message &seed) : ACommand(srv, seed, 1, 5)
{
	std::cerr << "Bare Userhost constructor called, hope that is not a problem..." << std::endl;
}

Userhost::~Userhost(void) {}

// Return RPL_USERHOST 302 for up to 5 NICKs
// This is one list with a space-separated parameter list of the userHostMsg ouptuts
// TODO Test with multiple NICKs
void	Userhost::executeCmd(void)
{
	std::list<std::string>	in_params = _msg.getParams();
	std::list<std::string>	o_params;
	while (!in_params.empty())
	{
		std::string	nick = in_params.front();
		// Find User by Nick
		User*	target = this->_srv->_findUserByNick(nick);
		// TODO This should *do* something with the return!
		if (target)
			o_params.push_back(target->getUserHostMsg());
		in_params.pop_front();
	}
	Message* reply;
	reply = Message::_replyNonNumeric(_msg);
	// TODO add o_params to reply
	reply->addParams(o_params);
	this->_responses.push(reply);
}
