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

void	 Ping::executeCmd(void)
{
	std::cout << "Here we alter the entities, queue messages, etc" << std::endl;
}
