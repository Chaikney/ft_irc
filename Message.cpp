#include "Message.hpp"
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>	// EOF marker in stringstream

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
Message::Message(std::string text_recvd) : _tags(""), _source(""), _command(""), _params()
{
    char	c;

    if (text_recvd.empty())
        throw std::invalid_argument("Tried to create message with empty string");
    if (text_recvd.length() < 3)
        throw std::invalid_argument("Message too short");
    if (text_recvd.length() > MSG_LEN)
        throw std::invalid_argument("Message too long");
    std::istringstream	strm(text_recvd);
    std::cout << "Message constructor called using:" << text_recvd << std::endl;
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
            // last parameter, read everything else as one and break
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
        this->_params.push_back(tmp);
    }
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

// NOTE Must be a DEEP COPY of _params
Message::Message(const Message &original): _tags(original._tags), _source(original._source),
										   _command(original._command), _params()
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
