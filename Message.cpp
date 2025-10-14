#include "Message.hpp"
#include "User.hpp"
#include "Channel.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>	// EOF marker in stringstream
#include <algorithm>	// erase

// Make sure that the string is not empty and it ends in crlf
// TODO Check the final two chars are cr and lf
// TODO Check that we have not received illegal characters
// TODO Check validity of the command parsed out
// TODO Is parsing command OK in case of no parameters? i.e. message ends?
// NOTE The four spaces "    " call gives weird output, not sure what it should give
// FIXED Ignore final spaces -- at the moment the parameter list ends as ~500 spaces
// ...something to do with null termination?
// FIXED Step over extra spaces between command and parameters (also check before command)
// FIXME Now the only parsing issue seems to be the final \n in cases where there are no end spaces
// ...we should strip that out
// TODO UNify the parsing part of these 2 constructors
// TODO Fill targets -- if blank it is for the Server?
Message::Message(std::string text_recvd) : _tags(""), _source(""),
										   _command(""), _params(),
										   _origin(), _targets()
{
    if (text_recvd.empty())
        throw std::invalid_argument("Tried to create message with empty string");
    if (text_recvd.length() < 3)
        throw std::invalid_argument("Message too short");
    if (text_recvd.length() > MSG_LEN)
		throw std::invalid_argument("Message too long");
	_parseMessage(text_recvd);
}

// This should be easy but it has not been
// TODO Move this to a shared "helpers" file?
void	stripFinalNewline(std::string *str)
{
	size_t	hunt_it = str->find('\n') ;
	if (hunt_it != std::string::npos)
	{
		// std::cout << "Newline found in this parameter:" << *str;
		// std::cout << ". At position: " << str->find('\n') << std::endl;
		str->erase(hunt_it);
//		str->erase(std::remove(str->begin(), str->end()), str->end());
//		std::cout << "It is now:" << *str << std::endl;
	}
	hunt_it = str->find('\r') ;
	if (hunt_it != std::string::npos)
	{
		str->erase(hunt_it);
	}
}

void	Message::_parseMessage(std::string text_recvd)
{
	char	c;
	std::istringstream	strm(text_recvd);

    c = strm.get();
    if (c == '@')
    {
        // we have been sent tags which we do not support read into buffer anyway
        std::getline(strm, this->_tags, ' ');
		_stepOver(strm);
        c = strm.get();
    }
    if (c == ':')
        {
        // we have been sent a source which clients should not do
        std::getline(strm, this->_source, ' ');
		_stepOver(strm);
		}
	else
		strm.unget();	// If that char is not identifying tags/source, we need to use it
    // After this, we have a command
	_stepOver(strm);
    std::getline(strm, this->_command, ' ');
    stripFinalNewline(&_command);
    // And the parameters, finally
	_stepOver(strm);
    std::string	tmp;
    while (strm)
    {
        c = strm.peek();
		if ((c == EOF) || (c == '\n'))
			break ;
        else if (c == ':')
        {
            // : indicates the last parameter, read everything else as one and break
			strm.ignore(1, ':');
            std::getline(strm, tmp, '\n');
        }
        else
        {
			// Store everything until the next space and ignore subsequent spaces
            std::getline(strm, tmp, ' ');
			_stepOver(strm);
        }
//		std::cout << "Adding param:" << tmp << std::endl;	// HACK to debug
		stripFinalNewline(&tmp);
        this->_params.push_back(tmp);
	}
}

Message::Message(std::string text_recvd, User *usr) : _tags(""), _source(""),
										   _command(""), _params(),
										   _origin(usr), _targets()
{
    if (text_recvd.empty())
        throw std::invalid_argument("Tried to create message with empty string");
    if (text_recvd.length() < 3)
        throw std::invalid_argument("Message too short");
    if (text_recvd.length() > MSG_LEN)
		throw std::invalid_argument("Message too long");
	_parseMessage(text_recvd);
}

// Constructor suited for *outward* Messages
// i.e. has parameters, is probably a numeric reply
// TODO Consider if this needs to take a User as well
// FIXME I think this can lead to the last parameter NOT having a :
Message::Message(std::string &src, std::string &cmd,
				 std::list<std::string> params, std::list<int> target) :
	_tags(""), _source(src), _command(cmd), _params(params), _origin(), _targets(target)
{
	std::cout << "Outward-facing Message constructed" << std::endl;
}

// Step over any spaces in a string stream
void	Message::_stepOver(std::istringstream &strm) const
{	char	c;

	c = strm.get();
	while (c == ' ')
		c = strm.get();
	strm.putback(c);
}

// Message Destructor should handle itself unless we add more things
Message::~Message(void)
{
//	this->_params.clear();
}

// Give any string, get back a Message object to use wherever
// TODO This has to handle failure to make a message somehow. Throw to next level?
Message	*Message::makeMessage(std::string &str)
{
	Message	*msg = new Message(str);
	return (msg);
}

// Make ONE Message from a provided string.
// If there are multiples in the string (i.e. more than one \n) that is a problem for
// the caller to solve.
Message	*Message::makeMessage(std::string &str, User *origin)
{
	// HACK debug
//	std::cout << "Attemtping to make Message from:" << str << std::endl;
	Message	*msg = new Message(str, origin);
	return (msg);
}
// NOTE Must be a DEEP COPY of _params, etc. Standard containers (probably?) take care of that for us.
Message::Message(const Message &original): _tags(original._tags), _source(original._source),
										   _command(original._command), _params(),
										   _origin(original._origin), _targets(original._targets)
{
	this->_params = original._params;
}

std::string	Message::getTags() const
{
	return(this->_tags);
}

std::string	Message::getSource() const
{
	return(this->_source);
}

std::string	Message::getCommand() const
{
	return(this->_command);
}

std::list<std::string>	Message::getParams() const
{
	return(this->_params);
}

// NOTE May return a NULL pointer, indicating that the Server sent it
User*	Message::getOrigin() const
{
	return (this->_origin);
}

std::list<int>	Message::getTargets() const
{
	return(this->_targets);
}

// Return the message's parameters as a string suitable to include in
// a transmittable form
// NOTE The final parameter is the only one allowed to contain spaces
// ...it must be preceded by :
//  FIXME A parameter of only a newline code leads to the : being added.
//  ...careful, an empty parameter at the end is OK (I think)
//  If there is only one, spaceless parameter, don't add the :
//  ...KVIRC JOIN gets confused by it i think.
std::string	Message::_paramToString(std::list<std::string> lst) const
{
	std::string	msg;
	int	n;

	if (lst.empty())
		return (msg);
	n = lst.size();
	std::list<std::string>::const_iterator  it = lst.begin();
	// NOTE Here special case adding : for single parameter with spaces
	if ((n == 1))
	{
		msg.append(*it);
		if (msg.find_first_of(" ") != std::string::npos)
			msg = (":" + msg);
		return (msg);
	}
	while (it != lst.end())
	{
		msg.append(" ");
		if (n == 1)
			msg.append(":");
		msg.append(*it);
		it++;
		n--;
	}
	return (msg);
}

// NOTE We are not sending tags, so we ignore them
// message ::= ['@' <tags> SPACE] [':' <source> SPACE] <command> <parameters> <crlf>
//  SPACE  ::=  %x20 *( %x20 )   ; space character(s)
//  crlf   ::=  %x0D %x0A        ; "carriage return" "linefeed"
//  TODO Test the output of this
std::string	Message::serialiseMsg(void) const
{
	std::string	msg;
	// if (!this->_tags.empty())
		// handle tags
	if (!this->_source.empty())
	{
		msg.append(":");
		msg.append(this->_source);
		msg.append(" ");
	}
	msg.append(this->_command);
	if (!this->_params.empty())
	{
		msg.append(" ");
		msg.append(_paramToString(this->_params));
	}
	msg.append("\r\n");
	return (msg);
}

// Add a list of parameters onto the end of the parameter list
// Return true on success
// TODO Add exceptions, error-checking
bool	Message::addParams(std::list<std::string> &addme)
{
	this->_params.splice(_params.end(), addme);
	return (true);
}

// Same as above but for one single parameter
bool	Message::addParams(std::string &addme)
{
	this->_params.push_back(addme);
	return (true);
}

// This is to get messages out to a whole channel
// I.e. inform of AWAY / QUIT; NOTICE or PRIVMSG
// Test on:
// [ ] JOIN
// [ ] PART
// [ ] AWAY
// [x] QUIT
// [ ] KICK (we haven't got KICK yet)
// [x] TOPIC
// NOTE Maybe this would be better taking a "params" list....
Message*	Message::_channelMessage(Message &msg, Channel *chan)
{
	Message*	transmit;
	std::string	src = msg.getOrigin()->getNick();
	std::string cmd_as_str = msg.getCommand();
	std::list<std::string>	params;
	// NOTE This call gets the FDs of all *except* the sender
	std::list<int>	targets = chan->getBroadcastFDs(msg.getOrigin());
	if (cmd_as_str.compare("TOPIC") == 0)
	{
		// HACK Adding back the originating FD so they get the message too.
		targets.push_back(msg.getOrigin()->getFD());
	 	params.push_back((chan->getName()));
	 	params.push_back((msg.getParams().back()));
	}
	else if (cmd_as_str.compare("JOIN") == 0)
		params.push_back((msg.getParams().back()));
	// else if (cmd_as_str.compare("PART") == 0)
	// 	params.push_back((msg.getParams().back()));
	// else if (cmd_as_str.compare("AWAY") == 0)
	// 	params.push_back((msg.getParams().back()));
	transmit = new Message(src, cmd_as_str, params, targets);
	std::cout << *transmit << std::endl;
	std::cout << transmit->serialiseMsg() << std::endl;
	return (transmit);
}

// Simplest possible reply construction,
// This is for commands like PING where we don't need to refer to channel or user
// No source
Message*	Message::_replyNonNumeric(Message &msg)
{
	Message*	transmit;
	std::string	src;
	std::string cmd_as_str = msg.getCommand();
	std::list<int>	targets;
	targets.push_front(msg.getOrigin()->getFD());
	std::list<std::string>	params;
	if (cmd_as_str.compare("PING") == 0)
	{
		cmd_as_str = "PONG";
		params.push_back(SERVERNAME);
		params.push_back((msg.getParams().back()));
	}
	else if (cmd_as_str.compare("QUIT") == 0)
	{
		cmd_as_str = "ERROR";	// NOTE This can *only* go to the one who called QUIT
	}
	transmit = new Message(src, cmd_as_str, params, targets);
	return (transmit);
}

// Return a Message in reply to a command, but a non-numeric one
// i.e. typically this will be the same command back to the place it came from
// Take JOIN as an example to test this
// https://modern.ircdocs.horse/#join-message
// ...how would you get the channel name?
// NOTE targets is a LIST so we can expand sending to multiple clients
// ...that is maybe not needed now?
// NOTE For this to work with PART you have to include the Reason, final parameter
// FIXME We send a double channel for JOIN here
Message*	Message::_replyNonNumeric(Message &msg, Channel *chan)
{
	Message*	transmit;
	std::string	src = msg.getOrigin()->getNick();
	std::string cmd_as_str = msg.getCommand();

	std::list<std::string>	params;
	params.push_back(chan->getName());
	if (cmd_as_str.compare("PART") == 0)
		params.push_back((msg.getParams().back()));
	// NOTE Join only needs the channel name when replying to a client
	// else if (cmd_as_str.compare("JOIN") == 0)
	// 	params.push_back((msg.getParams().back()));
	// If the command should not be sent to the sender, add as parameter
	// NOTE Be careful of confusing this with _channelMessage()!
	// TODO Check that this is used consistently
	std::list<int>	targets = chan->getBroadcastFDs();
//	targets.push_front(msg.getOrigin()->getFD());

	transmit = new Message(src, cmd_as_str, params, targets);
	std::cout << "Non-numeric reply composed" << std::endl;
	std::cout << *transmit << std::endl;
	std::cout << transmit->serialiseMsg() << std::endl;
	return (transmit);
}
// Use the Message and code to create a reply Message to be queued
// - source
// - command = reply code
// - parameters: add client by default (msg->usr-getNick())
// - use a switch to add other parameters
// DONE src here should be a Server class variable
// TODO More protection needed, i.e. on rep_code
// TODO Consider renaming this to be more specific
// NOTE When rep_code is < 100, it should be padded to 3 digits
Message*	Message::_reply(Message &msg, int rep_code)
{
	Message*	transmit;
	std::stringstream strm;
	strm << rep_code;
	std::string cmd_as_str = strm.str();
	if (cmd_as_str.length() == 1)
		cmd_as_str.insert(0, 2, '0');

	std::list<int>	targets;
	targets.push_front(msg.getOrigin()->getFD());

	std::string	src(SERVERNAME);

	std::list<std::string>	params;
	// TODO If this returns empty, put something else there
	params.push_back(msg.getOrigin()->getNick());

	switch (rep_code)
	{
		case RPL_WELCOME:
			params.push_back("Welcome to this network");
			break;
		case RPL_YOURHOST:
			params.push_back("Your host is:" + SERVERNAME);
			break;
		case RPL_CREATED:
			// FIXME Cannot now call getCreation...
//			params.push_back(this->getCreation());
			break;
		// TODO Update this when we have modes and versioning
		case RPL_MYINFO:
			params.push_back(SERVERNAME);
			params.push_back(VERSION);
			params.push_back("user_modes_here");
			params.push_back("channel_modes_here");
			break;
		case RPL_UNAWAY:
			params.push_back("Welcome back!");
			break;
		case RPL_NOWAWAY:
			params.push_back("Off you go then, bye.");
			break;
		case ERR_NOSUCHNICK:
			params.push_back(msg.getParams().front());	// HACK Careless assumption here
			params.push_back("No such nick or channel found");
			break;
		case ERR_CANNOTSENDTOCHAN:
			params.push_back("You do not have permission to send to this channel");
			break;
		case ERR_UNKNOWNCOMMAND:
			params.push_back(msg.getCommand());
			params.push_back("Command not known on this server");
			break;
		case RPL_LISTEND:
			params.push_back("End of /LIST");
			break;
		case ERR_NONICKNAMEGIVEN:
			params.push_back("No nickname given");
			break;
		case ERR_NICKNAMEINUSE:
			params.push_back(msg.getParams().front());	// TODO Get the name wanted
			params.push_back("Nickname already in use");
			break;
		case ERR_NOTREGISTERED:
			params.push_back("You have not registered");
			break;
		case ERR_NEEDMOREPARAMS:
			params.push_back(msg.getCommand());
			params.push_back("Not enough parameters");
			break;
		case ERR_ALREADYREGISTERED:
			params.push_back("You may not re-register");
			break;
		case ERR_PASSWORDMISMATCH:
			params.push_back("Password incorrect");
			break;
		case ERR_BADCHANMASK:
			params.push_back("Bad channel mask (i.e. name is not valid)");
			break;
		case ERR_CHANOPRIVSNEEDED:
			params.push_back("You're not channel operator");
			break;
		default:
			std::cerr << "Reply not handled yet:" << rep_code << std::endl;
	}
	std::cout << "Added " << params.size() << "parameters" <<std::endl;
	transmit = new Message(src, cmd_as_str, params, targets);
	return (transmit);
}

// Channel-including overload of the _reply method above.
// Needed for replies which refer to Channel characteristics (e.g. TOPIC)
// TODO The first part of this repeats from above and should be consolidated
// No padding needed for these commands though
Message*	Message::_reply(Message &msg, int rep_code, Channel *chan)
{
	Message*	transmit;
	std::stringstream strm;
	strm << rep_code;
	std::string cmd_as_str = strm.str();

	std::list<int>	targets;
	targets.push_front(msg.getOrigin()->getFD());

	std::string	src(SERVERNAME);

	std::list<std::string>	params;
	// TODO If this returns empty, put something else there
	params.push_back(msg.getOrigin()->getNick());

	// HACK this is only here for the RPL_LIST splicing that I couldn't call properly without it
	std::list<std::string> who = chan->getListInfo();
	switch (rep_code)
	{
	// NOTE catching this message needs an overload with Channel
		case RPL_TOPIC:
			params.push_back(chan->getName());
			params.push_back(chan->getTopic());
			break;
		case RPL_TOPICWHOTIME:
			params.push_back("TODO if we can get the channel we can get this I guess");
			break;
		case ERR_INVITEONLYCHAN:
			params.push_back(chan->getName());
			params.push_back("Cannot join channel (+i)");
			break;
		case RPL_LIST:
			params.splice(params.end(), who);
			break;
		case ERR_CHANOPRIVSNEEDED:
			params.push_back(chan->getName());
			params.push_back("You're not a channel operator");
			break;
		default:
			std::cerr << "Reply not handled yet:" << rep_code << std::endl;
	}
	std::cout << "Added " << params.size() << "parameters" <<std::endl;
	transmit = new Message(src, cmd_as_str, params, targets);
	return (transmit);
}

// And a User-taking overload
Message*	Message::_reply(Message &msg, int rep_code, User *usr)
{
	Message*	transmit;
	std::stringstream strm;
	strm << rep_code;
	std::string cmd_as_str = strm.str();

	// NOTE This is a list because Message constructors. Needs worked out!
	std::list<int>	target;
	target.push_back(msg.getOrigin()->getFD());

	std::string	src(SERVERNAME);

	std::list<std::string>	params;
	// TODO If this returns empty, put something else there
	params.push_back(msg.getOrigin()->getNick());
	std::list<std::string> who = usr->getWhoReply();

	switch (rep_code)
	{
		// FIXME This needs channel as well :'(
		case RPL_WHOREPLY:
//			params.push_back(usr->getWhoReply());
			params.splice(params.end(), who);
			break;
		default:
			std::cerr << "Reply not handled yet:" << rep_code << std::endl;
	}
	std::cout << "Added " << params.size() << "parameters" <<std::endl;
	transmit = new Message(src, cmd_as_str, params, target);
	return (transmit);
}
