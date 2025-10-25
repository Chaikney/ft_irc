#include "ACommand.hpp"
#include "Pass.hpp"
#include "Server.hpp"
#include "User.hpp"

# include <iostream>
# include <string>

// Pass::Pass(void)
// {
// 	std::cerr << "Bare Pass constructor should not be called" << std::endl;
// }

Pass::Pass(Server* srv, Message &seed) : ACommand(srv, seed, 1, 1)
{
	std::cerr << "Bare Pass constructor called, hope that is not a problem..." << std::endl;
}

Pass::~Pass(void) {}

// Command: PASS
// Parameters: <password>
void	Pass::executeCmd(void)
{
	// FIXME private member, how to check? Pass vlue to server and get a bool back
	std::string	cPass = _msg.getParams().front();
	User*	usr = _msg.getOrigin();

	if (this->_srv->checkPasswd(cPass))
	{
		std::cout << "Password match!" << std::endl;
		if (!(usr->isVerified()))
			usr->switchVerification();
		else
		{
			this->_responses.push(Message::_reply(_msg, ERR_ALREADYREGISTERED));
		}
		// TODO Server sends some kind of acknowledgment?
	}
	else
	{
		this->_responses.push(Message::_reply(_msg, ERR_PASSWORDMISMATCH));
		// TODO disconnect them by implementing the ERROR command
	}
}
