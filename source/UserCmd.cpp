#include "ACommand.hpp"
#include "UserCmd.hpp"
#include "Server.hpp"
#include "User.hpp"

# include <iostream>
# include <string>
# include <list>

// UserCmd::UserCmd(void)
// {
// 	std::cerr << "Bare UserCmd constructor should not be called" << std::endl;
// }

UserCmd::UserCmd(Server* srv, Message &seed) : ACommand(srv, seed, 4, 4)
{
	std::cerr << "Bare UserCmd constructor called, hope that is not a problem..." << std::endl;
}

UserCmd::~UserCmd(void) {}

// FIXME IF the nick name is already in use that doesn't seem to stop registration?
// TODO Sure there are other errors to catch here...
// FIXME Hexchat at least does not get given a Real Name
//      Command: USER
//  Parameters: <username> 0 * <realname>
void	UserCmd::executeCmd(void)
{
	User*	usr = this->_msg.getOrigin();
	std::list<std::string>	_params = this->_msg.getParams();
	// TODO Consider handling this in outer loop
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
		this->_srv->_sendWelcome(&_msg, usr);
}
