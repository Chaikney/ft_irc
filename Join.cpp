#include "Join.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

Join::Join(Server *srv, Message &msg) : ACommand(srv, msg) {}

Join::~Join() {}

// DONE Send acknowledgements per https://modern.ircdocs.horse/#join-message
// [X] A JOIN message with the client as the message <source> and the channel they have joined as the first parameter of the message.
// [X] The channel’s topic (with RPL_TOPIC (332) and optionally RPL_TOPICWHOTIME (333)), and no message if the channel does not have a topic.
// [X] A list of users currently joined to the channel (with one or more RPL_NAMREPLY (353) numerics followed by a single RPL_ENDOFNAMES (366) numeric).
// ....These RPL_NAMREPLY messages sent by the server MUST include the requesting client that has just joined the channel.
// DONE Break out the name normalisation to a helper function
// TODO JOIN can accept an alternative parameter of '0' = PART all the user's channels
// TODO Improve parameter handling so JOIN Can handle multiple Channels (comma separated)
// TODO To support KEY mode channels, the 2nd paramter is a password
// FIXED? the reply or broadcast message repeats the #channelname
void Join::executeCmd(void)
{
	User* usr = _msg.getOrigin();
    std::list<std::string> params = _msg.getParams();
    if (params.empty())
	{
		this->_responses.push(Message::_reply(_msg, ERR_NEEDMOREPARAMS));
        return ;
	}
    std::string chan = params.front();
	if (chan.compare("0") == 0)
	{
		std::cerr << "JOIN 0 not yet implemented." << std::endl;
	}
    // If the channel name is valid, store and remove from our params
	if (!Channel::normaliseChanName(&chan))
	{
		// Send error message and stop processing message
		this->_responses.push(Message::_reply(_msg, ERR_BADCHANMASK));
		return ;
	}
	else
    	params.pop_front();

	// If the channel cannot be found, create it
    Channel *channel = this->_srv->_findChannel(chan);
    if (!channel)
        channel = this->_srv->_createChannel(chan);

	// NOTE This logic is odd, why remove an invite? Just to keep the list clean?
	// FIXME I had this fail recently there is a problem somewhere.
	// TODO Encapsulate this in some kind of Channel:addUser method
    if (channel->isInviteOnly())
    {
        if (!channel->isInvited(usr->getNick()))
        {
			this->_responses.push(Message::_reply(_msg, ERR_INVITEONLYCHAN, channel));
            return ;
        }
        else
        {
            channel->removeInvite(usr->getNick());
        }
    }

	// Add member to channel
    if (channel->addMember(usr))
    {
        usr->addChannel(chan);
		// Send JOIN confirmation
		this->_responses.push(Message::_replyNonNumeric(_msg, channel));

		// Send topic if channel has one
		if (!channel->getTopic().empty())
		{
			this->_responses.push(Message::_reply(_msg, RPL_TOPIC, channel));
			this->_responses.push(Message::_reply(_msg, RPL_TOPICWHOTIME, channel));
		}

		// Send names list (shows who is in the channel)
		this->_responses.push(Message::_reply(_msg, RPL_NAMREPLY, channel));
		this->_responses.push(Message::_reply(_msg, RPL_ENDOFNAMES, channel));

        // Notify channel (simple join message, or should it be a NOTICE?)
        // "This message may be sent from a server to a client to notify the client
        // that someone has joined a channel. In this case, the message <source>
        // will be the client who is joining,
        // and <channel> will be the channel which that client has joined
		this->_responses.push(Message::_channelMessage(_msg, channel));
    }
	return ;
}
