#include "ACommand.hpp"
#include "Message.hpp"

# include <iostream>

// ACommand::ACommand(void) : _msg(0)
// {
// 	std::cerr << "Bare ACommand constructor should not be called" << std::endl;
// }

// IDEA Implement NOTICE command
ACommand::ACommand(Server *srv, Message &seed) : _srv(srv),
												 _msg(seed),
												 _cmd_as_str(""),
												 _minParam(0),
												 _maxParam(10),
												 _responses()
{
    this->_cmd_as_str = seed.getCommand();
	std::cerr << "Bare ACommand constructor called, hope that is not a problem..." << std::endl;
}

ACommand::ACommand(Server *srv, Message &seed, size_t min, size_t max) : _srv(srv),
																		 _msg(seed),
																		 _cmd_as_str(""),
																		 _minParam(min),
																		 _maxParam(max),
																		 _responses()
{
    this->_cmd_as_str = seed.getCommand();
	std::cerr << "ACommand constructor called with min/max overides" << std::endl;
}

ACommand::~ACommand(void) {}

bool	ACommand::numParamsOK(void) const
{
	size_t	n = this->_msg.getParamCount();
	if ((n >= this->_minParam) && (n <= this->_maxParam))
		return (true);
	else
		return (false);
}

std::queue<Message *>	ACommand::getResponses(void)
{
	return (this->_responses);
}
