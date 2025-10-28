#ifndef USERCMD_HPP
# define USERCMD_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	UserCmd : public ACommand
{
	public:
		UserCmd(Server* srv, Message &seed);

		virtual ~UserCmd(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
