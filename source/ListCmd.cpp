#include "ACommand.hpp"
#include "ListCmd.hpp"

# include <iostream>

// ListCmd::ListCmd(void)
// {
// 	std::cerr << "Bare ListCmd constructor should not be called" << std::endl;
// }

//  Command: LIST
//  Parameters: [<channel>{,<channel>}] [<elistcond>{,<elistcond>}]
ListCmd::ListCmd(Server* srv, Message &seed) : ACommand(srv, seed, 0, 2)
{
	std::cerr << "ListCmd constructor called" << std::endl;
}

ListCmd::~ListCmd(void) {}

// IDEA This should handle receiving a list of channels (comma-separated)
// IDEA Filter the channel list that we call before looping over and listing
// ...secret channels only visible to operators, etc
void	ListCmd::executeCmd(void)
{
	if (this->_msg.getParamCount() == 0)	// list all channels on server
	{
		this->_responses.push(Message::_reply(_msg, RPL_LISTSTART));
		std::map<std::string, Channel*>	chans = this->_srv->getChannels();
		for (std::map<std::string, Channel*>::const_iterator it = chans.begin();
			 it != chans.end(); ++it)
		{
			Channel *c = it->second;
			this->_responses.push(Message::_reply(_msg, RPL_LIST, c));
		}
		this->_responses.push(Message::_reply(_msg, RPL_LISTEND));
	}
	else	// List SELECTED channels only
	{
		std::string	chans = _msg.getParams().front();
		if (chans.empty())
			std::cerr << "Better handling for LIST params needed" << std::endl;
		else if (chans.find_first_of(",") != std::string::npos)	// IDEA Comma split needed
			std::cerr << "LIST with selected channels not implemented yet" << std::endl;
		else
		{
			Channel::normaliseChanName(&chans);
			Channel*	c = this->_srv->findChannel(chans);
			if (!c)
			{
				this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
				return;
			}
			else
			{
				this->_responses.push(Message::_reply(_msg, RPL_LISTSTART));
				this->_responses.push(Message::_reply(_msg, RPL_LIST, c));
				this->_responses.push(Message::_reply(_msg, RPL_LISTEND));
			}
		}
	}
}
