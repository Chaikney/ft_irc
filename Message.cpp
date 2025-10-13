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
	else if (cmd_as_str.compare("QUIT") == 0)
	 	params.push_back("Quit: " + (msg.getParams().back()));
	// else if (cmd_as_str.compare("PART") == 0)
	// 	params.push_back((msg.getParams().back()));
	// else if (cmd_as_str.compare("AWAY") == 0)
	// 	params.push_back((msg.getParams().back()));
	transmit = new Message(src, cmd_as_str, params, targets);
	std::cout << *transmit << std::endl;
	std::cout << transmit->serialiseMsg() << std::endl;
	return (transmit);
}
