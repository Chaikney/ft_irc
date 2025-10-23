#ifndef LISTCMD_HPP
# define LISTCMD_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	ListCmd : public ACommand
{
	public:
		ListCmd(Server* srv, Message &seed);

		virtual ~ListCmd(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
