#ifndef PING_HPP
# define PING_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Ping : public ACommand
{
	public:
		Ping(Server* srv, Message &seed);

		virtual ~Ping(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
