#ifndef TOPIC_HPP
# define TOPIC_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Topic : public ACommand
{
	public:
		Topic(Server* srv, Message &seed);

		virtual ~Topic(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
