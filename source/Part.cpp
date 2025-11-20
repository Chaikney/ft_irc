#include "Part.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

Part::Part(Server *srv, Message &msg) : ACommand(srv, msg) {}

Part::~Part() {}

// PART command
// - Check name and channel exists.
// - Remove User, send confirmation and to channel
// - Remove channel if now empty
// IDEA Nice to be able to handle *multiple* Channel PARTs (comma separated)
void Part::executeCmd(void)
{
    std::list<std::string> params =_msg.getParams();
	User*	usr = _msg.getOrigin();
	// If we can't find the User something has gone wrong and we silently ignore the command
	if (!usr)
	{
		std::cerr << "PART command from a non-existent (already gone?) user" << std::endl;
		return ;
	}
    std::string chan = params.front();
	if (!Channel::normaliseChanName(&chan))
	{
		// Send error message and stop processing message
		this->_responses.push(Message::_reply(_msg, ERR_BADCHANMASK));
		return ;
	}
	else
    	params.pop_front();

    Channel *channel = this->_srv->findChannel(chan);
    if (!channel)
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
        return ;
	}

    if (channel->removeMember(usr))
    {
		// Send a PART confirmation to the User
		this->_responses.push(Message::_replyNonNumeric(_msg, channel));
        usr->removeChannel(channel);
		// ...and to the Channel
		this->_responses.push(Message::_channelMessage(_msg, channel));
        // If channel is empty, remove it
        // NOTE This has to be the last reference to the channel in the command!
        if (channel->getMemberCount() == 0)
			this->_srv->removeChannel(channel->getName());	// IDEA could pass the pointer directly with an overload
    }
	else	// removeMember = false implies the user was not a member
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOTONCHANNEL, channel));
		return ;
	}
}
