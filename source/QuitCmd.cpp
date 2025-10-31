#include "ACommand.hpp"
#include "QuitCmd.hpp"
#include "Server.hpp"
#include "User.hpp"
#include "Channel.hpp"

# include <iostream>
# include <string>
# include <list>
# include <map>	// for the channel switching, which could be improved...

// QuitCmd::QuitCmd(void)
// {
// 	std::cerr << "Bare QuitCmd constructor should not be called" << std::endl;
// }

QuitCmd::QuitCmd(Server* srv, Message &seed) : ACommand(srv, seed, 0, 1)
{
	std::cerr << "Bare QuitCmd constructor called, hope that is not a problem..." << std::endl;
}

QuitCmd::~QuitCmd(void)
{
	std::cerr << "QuitCmd destructor called, hope that is not a problem..." << std::endl;
}

// On quit, send ERROR to the client
// broadcast QUIT to their channels
// remove them from all channels and clean up traces
// NOTE The ERROR probably has to act directly as the FD will disappear...
// TODO Test (refactor?) the user-removal logic
// - all channels (should be encapsulated in removeMember method)
// - Server listings (perhaps roll into ERROR)
// FIXME This does not cause the User to be removed from channels (at least in Konv.)
// ...i.e. still appearted in a WHOIS listing after Quit
void	QuitCmd::executeCmd(void)
{
    std::list<std::string> params = _msg.getParams();
	User*	usr = _msg.getOrigin();
	std::string	reason;
	if (params.empty())
		reason  = "Quit";
	else
		reason  = "Quit: " + params.back();

    // Get all channels the user is on and broadcast QUIT message
	// TODO This channel-getting could be more efficient
	std::map<std::string, Channel*>	_channels = this->_srv->getChannels();
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        Channel *channel = it->second;
        if (channel->isMember(usr))
        {
			Message *broadcast = Message::_channelMessage(_msg, channel);
			broadcast->addParams(reason);
			this->_responses.push(broadcast);
//            _broadcastToChannel(channel, usr->getFD(), quitMsg, false);
			// FIXME This has caused a segfault; must be protected.
            channel->removeMember(usr);
        }
    }
	// Then we call handleError to remove the User themselves from the Server
	// FIXME How to handle ERROR?
//	this->_srv->handleError(&_msg, usr);
}
