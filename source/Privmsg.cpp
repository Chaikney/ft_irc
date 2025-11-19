#include "ACommand.hpp"
#include "Privmsg.hpp"

# include "Channel.hpp"
# include "User.hpp"
#include "ReplyEnums.hpp"

// Command: PRIVMSG
// Parameters: <target>{,<target>} <text to be sent>
// <target> is the nickname of a client or the name of a channel.
// When the PRIVMSG message is sent from a server to a client and <target> starts with a dollar character ('$', 0x24), the message is a broadcast sent to all clients on one or multiple servers.
// Optional: If <target> is a channel name, it may be prefixed with one or more channel membership prefix character (@, +, etc) and the message will be delivered only to the members of that channel with the given or higher status in the channel. Servers that support this feature will list the prefixes which this is supported for in the STATUSMSG RPL_ISUPPORT parameter, and this SHOULD NOT be attempted by clients unless the prefix has been advertised in this token.
// Aquí va la lógica para enviar mensajes privados o a canales
// IDEA Could be further checks on sendability if had Invisible etc
// Sends a message to user(s) or channel(s)
// https://modern.ircdocs.horse/#privmsg-message
Privmsg::Privmsg(Server* srv, Message &seed) : ACommand(srv, seed, 2, 2)
{
	std::cerr << "Privmsg constructor called" << std::endl;
}

Privmsg::~Privmsg(void) {}

// If sent directly to  a user that is Away, reply RPL_AWAY
// If sent to a channel, check membership
// IDEA Split into Channel / User methods?
// IDEA Allow for multiple targets?
// FIXED Set up of chat in clients not really working
// ...this is so dumb.
void	Privmsg::executeCmd(void)
{
	User*	sender = this->_msg.getOrigin();
	if (!sender)
	{
		// this should not happen!
		return;
	}
	std::list<std::string> params = _msg.getParams();
	// NOTE if there are mulltiple targets, target will be comma-separated
	std::string target = params.front();
	params.pop_front();
	// The message to be sent is always going to be the final parameter
	std::string msg_text = params.back();
	if (target.empty())
	{
		this->_responses.push(Message::_reply(_msg, ERR_NORECIPIENT));
		return;
	}
	if (target.find_first_of(",") != std::string::npos)
	{
		// NOTE Can't find a defintion of ERR_TOOMANYTARGETS, so choosing this instead
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
		return;
	}
	if (msg_text.empty())
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOTEXTTOSEND));
		return;
	}
	// Channel message
	// NOTE This identification will have to change if we add prefix characters
	if ((target[0] == '#') || (target[0] == '&'))
	{
		Channel *channel = this->_srv->findChannel(target);
		if (!channel)
		{
			this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
			return ;
		}
		// IDEA Faster to find Channel membership directly with User? Or not?
		// Only allow if sender is member of the channel
		if (!channel->isMember(sender))
		{
			this->_responses.push(Message::_reply(_msg, ERR_CANNOTSENDTOCHAN));
			return ;
		}
		else
			this->_responses.push(Message::_channelMessage(_msg, channel));
	}
	else // Message to individual user
	{
		User *to = this->_srv->findUserByNick(target);
		if (to)
		{
			if (to->isAway())
				this->_responses.push(Message::_reply(_msg, RPL_AWAY, to));
			else
			{
				Message*	dm = Message::_replyThirdParty(_msg, to);
				// NOTE To workaround Hexchat private conversation losing the first word,
				// we prefix with a space.
				dm->addParams(" " + msg_text);
				this->_responses.push(dm);
			}
		}
		else
			this->_responses.push(Message::_reply(_msg, ERR_NOSUCHNICK));
	}
}
