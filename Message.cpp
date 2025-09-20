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
// FIXME Ignore final spaces -- at the moment the parameter list ends as ~500 spaces
// ...something to do with null termination?
Message::Message(std::string text_recvd) : _tags(""), _source(""), _command(""), _params()
{
    char	c;

    if (text_recvd.empty())
        throw std::invalid_argument("Tried to create message with empty string");
    if ((text_recvd.length() > MSG_LEN) || (text_recvd.length() < 3))
        throw std::invalid_argument("Message too short or too long");
    std::istringstream	strm(text_recvd);
    std::cout << "Message constructor called using:" << text_recvd << std::endl;
    c = strm.get();
    if (c == '@')
    {
        // we have been sent tags which we do not support read into buffer anyway
        std::getline(strm, this->_tags, ' ');
        c = strm.get();
    }
    if (c == ':')
        // we have been sent a source which clients should not do
        std::getline(strm, this->_source, ' ');
	else
		strm.unget();
    // After this, we have a command
    std::getline(strm, this->_command, ' ');
    // And the parameters, finally
    std::string	tmp;
    while (strm)
    {
        c = strm.peek();
		if (c == EOF)
			break ;
        else if (c == ':')
        {
			strm.ignore(1, ':');
            std::getline(strm, tmp, '\n');
            // last parameter, read everything else as one and break
        }
        else
        {
            std::getline(strm, tmp, ' ');
        }
//		std::cout << "Adding param:" << tmp << std::endl;	// HACK to debug
        this->_params.push_back(tmp);
    }
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
