#ifndef PASS_HPP
# define PASS_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Pass : public ACommand
{
	public:
		Pass(Server* srv, Message &seed);

		virtual ~Pass(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
