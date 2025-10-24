#include "Part.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

Part::Part(Server *srv, Message &msg) : ACommand(srv, msg) {}

Part::~Part() {}

void Part::executeCmd(void)
{
    std::string chan = params.front();
	if (!this->normaliseChanName(&chan))
	{
		// Send error message and stop processing message
		this->_toProcess.push(Message::_reply(*msg, ERR_BADCHANMASK));
		return ;
	}
	else
    	params.pop_front();

    Channel *channel = _findChannel(chan);
    if (!channel)
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOSUCHCHANNEL));
        return ;
	}

    if (channel->removeMember(usr))
    {
		// Send a PART confirmation to the User
		this->_toProcess.push(Message::_replyNonNumeric(*msg, channel));
        usr->removeChannel(chan);
		// ...and to the Channel
		this->_toProcess.push(Message::_channelMessage(*msg, channel));
        // If channel is empty, remove it
        if (channel->isEmpty())
            _removeChannel(chan);
    }
	else
	{
		this->_toProcess.push(Message::_reply(*msg, ERR_NOTONCHANNEL));
		return ;
	}
}
