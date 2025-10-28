#ifndef AWAY_HPP
# define AWAY_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Away : public ACommand
{
	public:
		Away(Server* srv, Message &seed);

		virtual ~Away(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
