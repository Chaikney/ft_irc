#ifndef USERHOST_HPP
# define USERHOST_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Userhost : public ACommand
{
	public:
		Userhost(Server* srv, Message &seed);

		virtual ~Userhost(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
