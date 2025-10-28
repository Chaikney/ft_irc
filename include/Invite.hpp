#ifndef INVITE_HPP
# define INVITE_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Invite : public ACommand
{
	public:
		Invite(Server* srv, Message &seed);

		virtual ~Invite(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
