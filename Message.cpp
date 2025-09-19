#include "Message.hpp"
#include <string>
#include <iostream>
#include <sstream>

// Make sure that the string is not empty and it ends in crlf
// TODO Check the final two chars are cr and lf
// TODO Check that we have not received illegal characters
// TODO Check validity of the command parsed out
// TODO Is parsing command OK in case of no parameters? i.e. message ends?
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
    // After this, we have a command
    std::getline(strm, this->_command, ' ');
    // And the parameters, finally
    std::string	tmp;
    while (strm)
    {
        c = strm.peek();
        if (c == ':')
        {
            std::getline(strm, tmp, '\n');
            // last parameter, read everything else as one and break
        }
        else
        {
            strm >> tmp;
        }
        this->_params.push_back(tmp);
    }
}

// Give any string, get back a Message object to use wherever
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

std::string	Message::getCommand() const
{
	return(this->_command);
}

std::list<std::string>	Message::getParams() const
{
	return(this->_params);
}
