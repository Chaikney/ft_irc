#include "ACommand.hpp"
#include "Privmsg.hpp"

# include <iostream>
# include "Channel.hpp"
#include "ReplyEnums.hpp"

// Privmsg::Privmsg(void)
// {
// 	std::cerr << "Bare Privmsg constructor should not be called" << std::endl;
// }

// Command: PRIVMSG
// Parameters: <target>{,<target>} <text to be sent>
// <target> is the nickname of a client or the name of a channel.
// When the PRIVMSG message is sent from a server to a client and <target> starts with a dollar character ('$', 0x24), the message is a broadcast sent to all clients on one or multiple servers.
// Optional: If <target> is a channel name, it may be prefixed with one or more channel membership prefix character (@, +, etc) and the message will be delivered only to the members of that channel with the given or higher status in the channel. Servers that support this feature will list the prefixes which this is supported for in the STATUSMSG RPL_ISUPPORT parameter, and this SHOULD NOT be attempted by clients unless the prefix has been advertised in this token.
// Aquí va la lógica para enviar mensajes privados o a canales
// TODO There are further checks needed on whether a message is allowed, see docs
// Sends a message to user(s) or channel(s)
// https://modern.ircdocs.horse/#privmsg-message
// FIXME Mesage formatting is wrong. sender? Message must be precede by :
Privmsg::Privmsg(Server* srv, Message &seed) : ACommand(srv, seed, 2, 2)
{
	std::cerr << "Privmsg constructor called" << std::endl;
}

Privmsg::~Privmsg(void) {}

void	Privmsg::executeCmd(void)
{
	// TODO Would this work?
	User*	sender = this->_msg.getOrigin();
	if (!sender)
	{
		// message is from server, treat as Broadcast
	}
	std::list<std::string> params = _msg.getParams();
	// NOTE if there are mulltiple targets, target will be comma-separated
	std::string target = params.front();
	params.pop_front();
	// The message to be sent is always going to be the final parameter
	std::string msg_text = params.back();
	if (target.empty())
	{
		// ERR_NORECIPIENT (411)
		// 	TODO Add text to message
		// 	"<client> :No recipient given (<command>)"
		this->_responses.push(Message::_reply(_msg, ERR_NORECIPIENT));
		return;
	}
	if (target.find_first_of(",") != std::string::npos)
	{
		this->_responses.push(Message::_reply(_msg, ERR_TOOMANYTARGETS));
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
		// Only allow if sender is member of the channel
		Channel *channel = this->_srv->_findChannel(target);
		if (!channel)
		{
			this->_responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
			return ;
		}
		// TODO Faster to find Channel membership directly with User? Or not?
		if (!channel->isMember(sender))
		{
			// 404 ERR_CANNOTSENDTOCHAN
			this->_responses.push(Message::_reply(_msg, ERR_CANNOTSENDTOCHAN));
			return ;
		}
		else
			_responses.push(Message::_channelMessage(_msg, channel));
	}
	else // Message to individual user
	{
		// TODO Need to change the text format in PRIVMSG e.g. source, or not?
		User *to = this->_srv->_findUserByNick(target);
		if (to)
		{
			// FIXME This won't work without yet another overload. (to what?)
//			_responses.push(Message::_replyNonNumeric(*msg, to));
		}
		//	_sendToFD(to->getFD(), text + "\r\n");
		else
			this->_responses.push(Message::_reply(_msg, ERR_NOSUCHNICK));
	}
}
