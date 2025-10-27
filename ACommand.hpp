#ifndef ACOMMAND_HPP
# define ACOMMAND_HPP

#include "Server.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "Channel.hpp"

// NOTE Causes compilation warning in Ping constructor, "unused"?
//static	size_t	MAXPARAM = 10;

// This should provide the interface for all the IRC commands we use.
// - actions on the Server, Users, clients, etc
// - generate error messages
// - generate channel broadcasts
// - generate acknowledgment / direct responses
class	ACommand
{
	private:
		ACommand(void);	// Don't anyone call this!
		ACommand(const ACommand& no_copy_con_please);
		ACommand& operator=(const ACommand& no_assignment_pls);

	protected:
		Server*		_srv;	// where the commands run; allows access to server-wide info like e channel listings.
		// NOTE This being a reference, what does it imply?
		Message&	_msg;	// The source of / trigger for the command
		std::string	_cmd_as_str;
		// Depends on cmd, really but if we override this in the constructor...
		size_t		_minParam;
		size_t		_maxParam;
		// We maybe should *differentiate* the type of responses?
		std::queue<Message *>	_responses;

		ACommand(Server *srv, Message &seed);	// only sub-commands can call it
		// Constructor overrding default paramter limits
		ACommand(Server *srv, Message &seed, size_t min, size_t max);

	public:
		virtual ~ACommand(void);

		// Here, all the meat from "handleWhatever" will go in the subcommands
		virtual void	executeCmd(void) = 0;

		// parse the parameters in the Message - are they OK for the command
		virtual bool	numParamsOK(void) const;	// This can be simple and shared among all commands

		// Here, fill a bunch of messages to be sent back from the Server
		// NOTE This maybe doesn't have to be 0, what is a basic response?
		// it *returns* the messages that the exec *stored* perhaps?
		virtual std::queue<Message *>	getResponses(void);

};
#endif
