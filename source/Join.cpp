#include "Join.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

Join::Join(Server *srv, Message &msg) : ACommand(srv, msg, 1, 2) {}

Join::~Join() {}

void	Join::_handleKeyChannels(void)
{
	std::cerr << "TODO Password-protected channels not supported by JOIN yet" << std::endl;
}

// Issue PART for all User's channels. What is the best way to do this?
void	Join::_handleJoinZero(void)
{
	std::cerr << "JOIN 0 not yet implemented." << std::endl;
}

// On successfully JOINing a Channel:
// Send topic if channel has one
// Send names list (shows who is in the channel)
void	Join::_welcomeToChannel(Channel *chan)
{
	if (!chan->getTopic().empty())
	{
		this->_responses.push(Message::_reply(_msg, RPL_TOPIC, chan));
		this->_responses.push(Message::_reply(_msg, RPL_TOPICWHOTIME, chan));
	}
	this->_responses.push(Message::_reply(_msg, RPL_NAMREPLY, chan));
	this->_responses.push(Message::_reply(_msg, RPL_ENDOFNAMES, chan));
}

// DONE Send acknowledgements per https://modern.ircdocs.horse/#join-message
// [X] A JOIN message with the client as the message <source> and the channel they have joined as the first parameter of the message.
// [X] The channel’s topic (with RPL_TOPIC (332) and optionally RPL_TOPICWHOTIME (333)), and no message if the channel does not have a topic.
// [X] A list of users currently joined to the channel (with one or more RPL_NAMREPLY (353) numerics followed by a single RPL_ENDOFNAMES (366) numeric).
// ....These RPL_NAMREPLY messages sent by the server MUST include the requesting client that has just joined the channel.
// DONE Break out the name normalisation to a helper function
// TODO JOIN can accept an alternative parameter of '0' = PART all the user's channels
// TODO Improve parameter handling so JOIN Can handle multiple Channels (comma separated)
// TODO To support KEY mode channels, the 2nd parameter is a password
// FIXED? the reply or broadcast message repeats the #channelname
void Join::executeCmd(void)
{
    std::list<std::string> params = _msg.getParams();
    if (params.empty())
	{
		this->_responses.push(Message::_reply(_msg, ERR_NEEDMOREPARAMS));
        return ;
	}
    std::string chan = params.front();
	if (params.size() == 2)
		_handleKeyChannels();
	else if (chan.compare("0") == 0)
		_handleJoinZero();
	else if (chan.find(',') != std::string::npos)
		std::cerr << "multi channel JOIN not yet implemented" << std::endl;
	else	// we have one or more apparent channel names, happy path
	{
		// If the channel name is valid, store and remove from our params
		// TODO This should be a comma list
		if (!Channel::normaliseChanName(&chan))
		{
			// Send error message and stop processing message
			this->_responses.push(Message::_reply(_msg, ERR_BADCHANMASK));
			return ;
		}
		else
			params.pop_front();	// NOTE Would not work for comma list

		User* usr = _msg.getOrigin();
		// If the channel cannot be found, create it
		Channel *channel = this->_srv->_findChannel(chan);
		if (!channel)
			channel = this->_srv->_createChannel(chan);
		// Add member to channel
		if (channel->addMember(usr))
		{
			usr->addChannel(channel);
			// Send JOIN confirmation to sender and other members
			// Notify channel (simple join message, or should it be a NOTICE?)
			// "This message may be sent from a server to a client to notify the client
			// that someone has joined a channel. In this case, the message <source>
			// will be the client who is joining,
			// and <channel> will be the channel which that client has joined
			// TODO Why not make this a single Message, contents are identical.
			this->_responses.push(Message::_replyNonNumeric(_msg, channel));
			this->_responses.push(Message::_channelMessage(_msg, channel));
			_welcomeToChannel(channel);
		}
		else	// failed to join channel, find out and notify why
		{
			if (!channel->isInvited(usr->getNick()))
				this->_responses.push(Message::_reply(_msg, ERR_INVITEONLYCHAN, channel));
			if (channel->isBanned(usr->getNick()))
				this->_responses.push(Message::_reply(_msg, ERR_BANNEDFROMCHAN, channel));
		}
	}
	return ;
}
