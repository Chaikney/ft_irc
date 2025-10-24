#include "Names.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"

Names::Names(Server *srv, Message &msg) : ACommand(srv, msg) {}

Names::~Names() {}

// TODO There should be ome filtering of visible names based on user modes
void Names::executeCmd(void)
{
    std::list<std::string> params =_msg.getParams();

	if (params.empty())
	{
		this->_responses.push(Message::_reply(_msg, ERR_NEEDMOREPARAMS));
		return ;
	}
	while (!params.empty())
	{
		std::string	cname = params.front();
		this->_srv->normaliseChanName(&cname);
		Channel*	target = this->_srv->_findChannel(cname);
		if (target)
		{
			this->_responses.push(Message::_reply(_msg, RPL_NAMREPLY, target));
			this->_responses.push(Message::_reply(_msg, RPL_ENDOFNAMES, target));
		}
		else	// send end of names without touching channel
			// NOTE This triggers a false "not implemented 366" error message
		{
			Message*	no_channel = Message::_reply(_msg, RPL_ENDOFNAMES);
			no_channel->addParams(cname);
			this->_responses.push(no_channel);
		}
		params.pop_front();
	}
}
