#include "Names.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"

// TODO Check or confirm the parameter ranges for NAMES commands
Names::Names(Server *srv, Message &msg) : ACommand(srv, msg) {}

Names::~Names() {}

/// https://modern.ircdocs.horse/#names-message
// Read msg parameters and call to each named channel
// - for each channel:
// -- enumerate users
// -- send RPL_NAMEREPLY per user
// -- send RPL_ENDOFNAMES with the channel name
// TODO Test that this works with multiple channels
// TODO There should be some filtering of visible names based on user modes
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
		Channel::normaliseChanName(&cname);
		Channel*	target = this->_srv->_findChannel(cname);
		if (target)
		{
			this->_responses.push(Message::_reply(_msg, RPL_NAMREPLY, target));
			this->_responses.push(Message::_reply(_msg, RPL_ENDOFNAMES, target));
		}
		else	// send end of names without touching channel
		{
			Message*	no_channel = Message::_reply(_msg, RPL_ENDOFNAMES);
			no_channel->insertParam(cname);
			this->_responses.push(no_channel);
		}
		params.pop_front();
	}
}
