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
// FIXME Clients get duplicate messages regarding users if they are in >1 channel with them
// ...Konv puts them in the same channel / duplicates them
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
	std::set<Channel*>	_channels = usr->getMemberships();
	std::set<Channel *>::iterator	it = _channels.begin();
	while (it != _channels.end())
	{
		Channel* to_leave = *it;
		Message *broadcast = Message::_channelMessage(_msg, to_leave);
		broadcast->addParams(reason);
		this->_responses.push(broadcast);
		to_leave->removeMember(usr);
		usr->removeChannel(to_leave);
		it++;
	}
	// Then we call handleError to remove the User themselves from the Server
	// FIXME How to handle ERROR?
//	this->_srv->handleError(&_msg, usr);
}
