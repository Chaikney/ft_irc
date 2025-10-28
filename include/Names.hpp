#ifndef NAMES_HPP
# define NAMES_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Names : public ACommand
{
	public:
		Names(Server* srv, Message &seed);

		virtual ~Names(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
