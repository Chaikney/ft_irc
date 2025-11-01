#include "ACommand.hpp"
#include "Whois.hpp"
#include "Channel.hpp"
#include "User.hpp"
#include <list>

# include <iostream>

// AwaAwayite(void)
// {
// 	std::cerr << "Bare Whois constructor should not be called" << std::endl;
// }

// NOTE Hardcoded 10 as the max parameters, perhaps we should have an overal limit?
Whois::Whois(Server* srv, Message &seed) : ACommand(srv, seed, 1, 10)
{
	std::cerr << "Bare Whois constructor called, hope that is not a problem..." << std::endl;
}

Whois::~Whois(void) {}

//      Command: WHOIS
//  Parameters: [<target>] <nick>
//  TODO implement WHOIS command, various responses terminated with RPL_ENDOFWHOIS
//  this might involve:
    // ERR_NOSUCHNICK (401)	-- already got
    // ERR_NOSUCHSERVER (402)	-- all servers are ours, so not doing this one
    // ERR_NONICKNAMEGIVEN (431)--	already implmemented
    // RPL_WHOISCERTFP (276)	--	depends on SSL which are not implementing
    // RPL_WHOISREGNICK (307)	--	checks user.isRegistered
    // RPL_WHOISUSER (311)	--	we have user.getWhoIs for this
    // RPL_WHOISSERVER (312)--	adds server info, probably not needed
    // RPL_WHOISOPERATOR (313)--	checks if user is an operator (for the server? we don't have any)
    // RPL_WHOISIDLE (317)	--	we aren't really tracking idle time
    // RPL_WHOISCHANNELS (319)--	list channels the user is on
    // RPL_WHOISSPECIAL (320)
    // RPL_WHOISACCOUNT (330)	-	not needed because we are not doing accounts
    // RPL_WHOISACTUALLY (338)
    // RPL_WHOISHOST (378)	--	verty similar to WHOISSERVER...
    // RPL_WHOISMODES (379)	--	maybe later if we develop modes fully
    // RPL_WHOISSECURE (671)	--	no one will be using a secure connection, ignore it
    // RPL_AWAY (301)	--	this seems easy to do
    // FIXME Hexchat sends 2 parameters for a self-check and we don't cope with that
    // FIXME WHOIS may be sent with @preceding the nick, check that
void	Whois::executeCmd(void)
{
	std::string	inick =_msg.getParams().front();
	if (!User::normaliseNick(&inick))
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHNICK));
		return ;
	}
	if (!this->_srv->_isNickTaken(inick))
	{
		this->_responses.push(Message::_reply(_msg, ERR_NOSUCHNICK));
		return ;
	}
	User*	target = this->_srv->_findUserByNick(inick);
	if (target)
	{
		this->_responses.push(Message::_reply(_msg, RPL_WHOISUSER, target));
		if (target->isAway())
			this->_responses.push(Message::_reply(_msg, RPL_AWAY, target));
	}
	this->_responses.push(Message::_reply(_msg, RPL_ENDOFWHOIS));
}
