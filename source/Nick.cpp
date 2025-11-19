#include "ACommand.hpp"
#include "Nick.hpp"
#include "Server.hpp"
#include "User.hpp"

# include <iostream>
# include <string>
# include <list>

Nick::Nick(Server* srv, Message &seed) : ACommand(srv, seed, 1, 1)
{
	std::cout << "Nick constructor called" << std::endl;
}

Nick::~Nick(void) {}

// Get user
// Get parameters
// Check the requested nick is valid and does not already exist
// Set new nickname on User (will need a setter on User?)
// IDEA Could acknowledge successful NICK:
// The NICK message may be sent from the server to clients to acknowledge their
// NICK command was successful, and to inform other clients about the change of nickname.
// In these cases, the <source> of the message will be the old nickname
// [ [ "!" user ] "@" host ] of the user who is changing their nickname.
void	Nick::executeCmd(void)
{
	std::list<std::string>	_params = _msg.getParams();
	User*	usr = this->_msg.getOrigin();
	std::string	newNick = _params.front();
	std::cout << "Trying to set nickname to " << newNick << std::endl;
	// NOTE These characters are forbidden from starting the nick
	std::string	notLeading = "#:&123456789";
	std::string	forbidden = " \b\n\r";

	if ((newNick.find_first_of(notLeading) == 0) ||
		(newNick.find_first_of(forbidden) != std::string::npos))
	{
		this->_responses.push(Message::_reply(_msg, ERR_ERRONEUSNICKNAME));
	}
	else if (this->_srv->isNickTaken(newNick, usr->getFD()))
	{
		this->_responses.push(Message::_reply(_msg, ERR_NICKNAMEINUSE));
	}
	else
	{
		std::cout << "setting nickname to " << newNick << std::endl;
		bool wasRegistered = usr->isRegistered();
		usr->setNick(newNick);
		// If user just became registered (had USER but was missing NICK), send welcome
		if (!wasRegistered && usr->isRegistered())
			this->_srv->sendWelcome(&_msg);
	}
}
