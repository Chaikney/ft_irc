#ifndef MODE_HPP
# define MODE_HPP

#include "ACommand.hpp"

#include <string>

// // Simplest possible test case?
// // Although it replies PONG, not PING...
class	Mode : public ACommand
{
	public:
		Mode(Server* srv, Message &seed);

		virtual ~Mode(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		// NOTE No parameters as they are in ACommand->msg
		virtual void	executeCmd(void);

	private:
		void	_userMode(Message *msg, User *usr, std::string target);
		void	_channelMode(Message *msg, User *usr, std::string target);
		void	_sendBanList(Channel* chan);

};
#endif
