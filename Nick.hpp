#ifndef NICK_HPP
# define NICK_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Nick : public ACommand
{
	public:
		Nick(Server* srv, Message &seed);

		virtual ~Nick(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
