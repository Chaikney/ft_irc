#include "ACommand.hpp"
#include "Ping.hpp"

# include <iostream>

// Ping::Ping(void)
// {
// 	std::cerr << "Bare Ping constructor should not be called" << std::endl;
// }

Ping::Ping(Message &seed) : ACommand(seed)
{
	std::cerr << "Bare Ping constructor called, hope that is not a problem..." << std::endl;
}

Ping::~Ping(void) {}

void	Ping::executeCmd(void)
{
	Message	msg = this->_msg;
    std::list<std::string> params = this->_msg.getParams();
    // if (params.empty())
    // {
    //     this->_responses.push(Message::_reply(msg, ERR_NEEDMOREPARAMS));
    //     return ;
    // }
    std::string origin = params.front();
    if (origin.empty())
    {
        this->_responses.push(Message::_reply(msg, ERR_NOORIGIN));
        return ;
    }
	// HACK send NULL for lack of overload, dangerous!
	// TODO This pushes too much logic to the Message class;
	//  better get a blank message then add to it here.
	this->_responses.push(Message::_replyNonNumeric(msg));
}
