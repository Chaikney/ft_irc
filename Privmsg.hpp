#ifndef PRIVMSG_HPP
# define PRIVMSG_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Privmsg : public ACommand
{
	public:
		Privmsg(Server* srv, Message &seed);

		virtual ~Privmsg(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
