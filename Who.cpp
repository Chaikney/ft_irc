#include "ACommand.hpp"
#include "Who.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// AwaAwayite(void)
// {
// 	std::cerr << "Bare Who constructor should not be called" << std::endl;
// }

Who::Who(Server* srv, Message &seed) : ACommand(srv, seed, 1, 1)
{
	std::cerr << "Bare Who constructor called, hope that is not a problem..." << std::endl;
}

Who::~Who(void) {}

//    Command: WHO
// Parameters: <mask>
// The parameter is either a NICK or a Channel name (we can ignore wildcards)
// Reply with multiple 352 terminated by RPL_ENDOFWHO (315)
void	Who::executeCmd(void)
{
	std::list<std::string>	params = _msg.getParams();
	std::string	mask = params.front();
	if (mask.empty())
	{
		this->_responses.push(Message::_reply(_msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	if (mask.find_first_of("#&") == 0)
	{
		// treat as Channel. Return all members of that Channel
		Channel*	target = this->_srv->getChannel(mask);
		if (!target)
			std::cerr << "Oops channel not found what we do?" << std::endl;
		else
		{
			std::set<User *>	users = target->getMembers();
			std::set<User *>::const_iterator it = users.begin();
			while (it != users.end())
			{
				User*	user = *it;
			 	this->_responses.push(Message::_reply(_msg, RPL_WHOREPLY, user));
				it++;
			}
//			std::cerr << "WHO for channels not implemented yet" << std::endl;
			// // FIXME Needs channel in the params, this solution doesnt cut it
			// // FIXME This is a C++11 form
			// for (User* user : users)
			// {
			// 	this->_responses.push(Message::_reply(_msg, RPL_WHOREPLY, user));
			// }
		}
	}
	else // treating it as a NICK
	{
		User*	user = this->_srv->_findUserByNick(mask);
		if (!user)
			this->_responses.push(Message::_reply(_msg, ERR_NOSUCHNICK, user));
		else
			this->_responses.push(Message::_reply(_msg, RPL_WHOREPLY, user));
	}
	// send final 315
	this->_responses.push(Message::_reply(_msg, RPL_ENDOFWHO));
}
