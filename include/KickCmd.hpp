#ifndef KICKCMD_HPP
# define KICKCMD_HPP

#include "ACommand.hpp"

class KickCmd : public ACommand
{
	public:
		KickCmd(Server *srv, Message &msg);
		virtual ~KickCmd();

		virtual void executeCmd(void);

	private:
		User*		_checkUser(std::string nick);
		Channel*	_checkChannel(std::string str);
		bool		_checkCombo(User *target, Channel *chan, User *usr);
};
#endif
