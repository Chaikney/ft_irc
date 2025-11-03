#ifndef JOIN_HPP
# define JOIN_HPP

#include "ACommand.hpp"

// Simplest possible test case?
// Although it replies PONG, not PING...
class	Join : public ACommand
{
	public:
		Join(Server* srv, Message &seed);

		virtual ~Join(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

	private:
		bool	_handleKeyChannels(Channel* locked, std::string key);
		void	_handleJoinZero(void);
		void	_welcomeToChannel(Channel *chan);
};
#endif
