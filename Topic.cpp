#include "Topic.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

Topic::Topic(Server *srv, Message &msg) : ACommand(srv, msg) {}

Topic::~Topic() {}

// TODO There should be ome filtering of visible names based on user modes
// FIXME Blank topic (RPL_TOPIC?) does not supply channel name (KVIRC)
// The broadcast message must include the channel name (and goes to all in channel)
// FIXME Parsing not putting the : in the correct place (sometimes?)
void Topic::executeCmd(void)
{
    std::list<std::string> params =_msg.getParams();
    std::string chan = params.front();
    Channel *channel = this->_srv->_findChannel(chan);
	User*	usr = _msg.getOrigin();
    if (!channel)
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
		return;
	}
    if (params.size() == 1)
    {
		this->_responses.push(Message::_reply(_msg, RPL_TOPIC));
        return ;
    }
    if (channel->isTopicProtected() && !channel->isOperator(usr->getFD()))
    {
		this->_responses.push(Message::_reply(_msg, ERR_CHANOPRIVSNEEDED));
        return ;
    }
    params.pop_front();
    std::string newTopic = params.front();
    channel->setTopic(newTopic, usr->getNick());
	this->_responses.push(Message::_channelMessage(_msg, channel));
}
