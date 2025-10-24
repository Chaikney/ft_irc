#ifndef WHO_HPP
# define WHO_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Who : public ACommand
{
	public:
		Who(Server* srv, Message &seed);

		virtual ~Who(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
