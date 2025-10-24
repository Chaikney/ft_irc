#include "Part.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

Part::Part(Server *srv, Message &msg) : ACommand(srv, msg) {}

Part::~Part() {}

void Part::executeCmd(void)
{
    std::list<std::string> params =_msg.getParams();
	User*	usr = _msg.getOrigin();
    std::string chan = params.front();
	if (!this->_srv->normaliseChanName(&chan))
	{
		// Send error message and stop processing message
		this->_responses.push(Message::_reply(_msg, ERR_BADCHANMASK));
		return ;
	}
	else
    	params.pop_front();

    Channel *channel = this->_srv->_findChannel(chan);
    if (!channel)
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
        return ;
	}

    if (channel->removeMember(usr))
    {
		// Send a PART confirmation to the User
		this->_responses.push(Message::_replyNonNumeric(_msg, channel));
        usr->removeChannel(chan);
		// ...and to the Channel
		this->_responses.push(Message::_channelMessage(_msg, channel));
        // If channel is empty, remove it
        if (channel->isEmpty())
            this->_srv->_removeChannel(chan);
    }
	else
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOTONCHANNEL));
		return ;
	}
}
