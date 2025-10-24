#ifndef PART_HPP
# define PART_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Part : public ACommand
{
	public:
		Part(Server* srv, Message &seed);

		virtual ~Part(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
