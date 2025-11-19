#include "ACommand.hpp"
#include "UserCmd.hpp"
#include "Server.hpp"
#include "User.hpp"

# include <iostream>
# include <string>
# include <list>

UserCmd::UserCmd(Server* srv, Message &seed) : ACommand(srv, seed, 4, 4)
{}

UserCmd::~UserCmd(void) {}

// Command: USER
// Parameters: <username> 0 * <realname>
void	UserCmd::executeCmd(void)
{
	User*	usr = this->_msg.getOrigin();
	std::list<std::string>	_params = this->_msg.getParams();
	if (usr->isRegistered())
	{
		this->_responses.push(Message::_reply(_msg, ERR_ALREADYREGISTERED));
		return ;
	}
	std::string	newUser = _params.front();
	// Skip to the final entry (used the first, ignore the middle 2)
	std::string	newRName = _params.back();
	if (newUser.empty())
		newUser = usr->getNick();
	if (newRName.empty())
		newRName = newUser;
	usr->setUser(newUser);
	usr->setReal(newRName);
	std::cout << "User: " << newUser << ", Really: " << newRName << std::endl;
	// Only send welcome bundle if user is fully registered (has valid nickname)
	if (usr->isRegistered())
		this->_srv->sendWelcome(&_msg);
}
