#ifndef WHOIS_HPP
# define WHOIS_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Whois : public ACommand
{
	public:
		Whois(Server* srv, Message &seed);

		virtual ~Whois(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

};
#endif
