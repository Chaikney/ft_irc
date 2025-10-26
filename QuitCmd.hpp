#ifndef QUITCMD_HPP
# define QUITCMD_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	QuitCmd : public ACommand
{
	public:
		QuitCmd(Server* srv, Message &seed);

		virtual ~QuitCmd(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
