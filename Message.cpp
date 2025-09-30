#include "Message.hpp"
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
Message::Message(std::string text_recvd) : _tags(), _source(""),
										   _command(""), _params(), _trailing(""),
										   _origin(NULL), _isNumeric(false)
{
    if (text_recvd.empty())
        throw std::invalid_argument("Tried to create message with empty string");
    // Allow very short messages (e.g., single-letter + newline)
    if (text_recvd.length() < 1)
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
	int		c;
	std::istringstream	strm(text_recvd);

    c = strm.get();
    if (c == EOF)
        return; // Empty message
    if (c == '@')
    {
        // Parse IRCv3 tags
        std::string tagString;
        std::getline(strm, tagString, ' ');
        _parseTags(tagString);
		_stepOver(strm);
        c = strm.get();
        if (c == EOF)
            return;
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
    
    // Check if command is numeric
    _isNumeric = (_command.length() == 3 && 
                  _command[0] >= '0' && _command[0] <= '9' &&
                  _command[1] >= '0' && _command[1] <= '9' &&
                  _command[2] >= '0' && _command[2] <= '9');
    
    // And the parameters, finally
	_stepOver(strm);
    std::string	tmp;
    while (strm.good())
    {
        c = strm.peek();
		if ((c == EOF) || (c == '\n'))
			break ;
        else if (c == ':')
        {
            // : indicates the last parameter, read everything else as one and break
			strm.ignore(1, ':');
            std::getline(strm, _trailing, '\n');
            stripFinalNewline(&_trailing);
            break;
        }
        else
        {
			// Store everything until the next space and ignore subsequent spaces
            std::getline(strm, tmp, ' ');
            if (!tmp.empty())
            {
                stripFinalNewline(&tmp);
                this->_params.push_back(tmp);
            }
			_stepOver(strm);
        }
//		std::cout << "Adding param:" << tmp << std::endl;	// HACK to debug
	}
}

Message::Message(std::string text_recvd, User *usr) : _tags(), _source(""),
										   _command(""), _params(), _trailing(""),
										   _origin(usr), _isNumeric(false)
{
    if (text_recvd.empty())
        throw std::invalid_argument("Tried to create message with empty string");
    // Allow very short messages (e.g., single-letter + newline)
    if (text_recvd.length() < 1)
        throw std::invalid_argument("Message too short");
    if (text_recvd.length() > MSG_LEN)
		throw std::invalid_argument("Message too long");
	_parseMessage(text_recvd);
}

// Step over any spaces in a string stream
void	Message::_stepOver(std::istringstream &strm) const
{	int		c;

	c = strm.get();
	while (c == ' ' && c != EOF)
		c = strm.get();
	if (c != EOF)
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

Message	*Message::makeMessage(std::string &str, User *origin)
{
	Message	*msg = new Message(str, origin);
	return (msg);
}
// NOTE Must be a DEEP COPY of _params
Message::Message(const Message &original): _tags(original._tags), _source(original._source),
										   _command(original._command), _params(), _trailing(original._trailing),
										   _origin(original._origin), _isNumeric(original._isNumeric)
{
	this->_params = original._params;
}

// New getters
const std::map<std::string, std::string>& Message::getTags() const
{
	return _tags;
}

std::string	Message::getTag(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = _tags.find(key);
	if (it != _tags.end())
		return it->second;
	return "";
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

std::string	Message::getTrailing() const
{
	return _trailing;
}

// NOTE May return a NULL pointer
User*	Message::getOrigin() const
{
	return (this->_origin);
}

bool	Message::isNumeric() const
{
	return _isNumeric;
}

// Setters
void	Message::setSource(const std::string &source)
{
	_source = source;
}

void	Message::setCommand(const std::string &command)
{
	_command = command;
	_isNumeric = (_command.length() == 3 && 
                  _command[0] >= '0' && _command[0] <= '9' &&
                  _command[1] >= '0' && _command[1] <= '9' &&
                  _command[2] >= '0' && _command[2] <= '9');
}

void	Message::addParam(const std::string &param)
{
	_params.push_back(param);
}

void	Message::setTrailing(const std::string &trailing)
{
	_trailing = trailing;
}

// Utility methods
std::string	Message::toString() const
{
	std::string result;
	
	// Add tags if any
	if (!_tags.empty())
	{
		result += "@";
		for (std::map<std::string, std::string>::const_iterator it = _tags.begin(); it != _tags.end(); ++it)
		{
			if (it != _tags.begin())
				result += ";";
			result += it->first;
			if (!it->second.empty())
				result += "=" + it->second;
		}
		result += " ";
	}
	
	// Add source if any
	if (!_source.empty())
		result += ":" + _source + " ";
	
	// Add command
	result += _command;
	
	// Add parameters
	for (std::list<std::string>::const_iterator it = _params.begin(); it != _params.end(); ++it)
	{
		result += " " + *it;
	}
	
	// Add trailing if any
	if (!_trailing.empty())
		result += " :" + _trailing;
	
	return result;
}

bool	Message::isValid() const
{
	return !_command.empty() && _command.length() <= 15;
}

// Helper method for parsing tags
void	Message::_parseTags(const std::string &tagString)
{
	std::istringstream strm(tagString);
	std::string tag;
	
	while (std::getline(strm, tag, ';'))
	{
		size_t pos = tag.find('=');
		if (pos != std::string::npos)
		{
			std::string key = tag.substr(0, pos);
			std::string value = tag.substr(pos + 1);
			_tags[key] = value;
		}
		else
		{
			_tags[tag] = "";
		}
	}
}
