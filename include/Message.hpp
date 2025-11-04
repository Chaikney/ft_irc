#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <list>
#include <iostream>

class	User;
// NOTE use include not forward definition so the "friend" keyword works
// class Server;
#include "Server.hpp"

// Static member variable. All messages of the same max length bytes
const int	MSG_LEN = 512;

// Each instance of this holds a message received by, or to be sent by, the Server
// They can be constructed from a single string, or with the various parts
// serialiseMsg() converts it back to a IRC-friendly format
// TODO We need something to check that the message is complete and logical
class	Message
{
	private:
		std::string				_tags;		// Unsupported but included for completion. IF we supported these, they should be stored as multi?MAP of strings
		std::string				_source;	// On outgoing messages, this is typically the User whose action is being responded to.
		std::string				_command;	// Used to identify what ACommand derivative to instantiate
		std::list<std::string>	_params;	// These vary with the message type
		User*					_origin;	// Link to sending User. If NULL, this is an outbound message.
		std::list<int>			_targets;	// FDs to which the Message should be sent

		// Constructor and overload that we may not use
		Message(void);	// private so not called - no blank messages please
		Message		operator=(const Message &irc);	// NOTE Not sure about assignment to a Message, this could be public

		// Helpers used in parsing messages
		void		_stepOver(std::istringstream &strm) const;
		void		_parseMessage(std::string text_recvd);
		std::string	_paramToString(std::list<std::string> lst) const;	// NOTE Helper for making transmittable messages

	public:
		// Constructors of various types; review to make sure they're all needed.
		// In practical use, these would all be called via one of the makeMessage type methods
		Message(std::string raw_text);
		Message(std::string text_recvd, User *usr);
		Message(std::string &src, std::string &cmd, std::list<std::string> params, std::list<int> targets);
		Message(const Message &irc);	// Copying a Message seems reasonable to allow
		~Message(void);

		// Turn a received string into a formatted Message for queuing
		static Message*			makeMessage(std::string &str);
		static Message*			makeMessage(std::string &str, User *origin);

		// Getters
		std::string				getTags() const;
		std::string				getSource() const;
		std::string				getCommand() const;
		std::list<std::string>	getParams() const;
		int						getParamCount() const;
		User*					getOrigin() const;
		std::list<int>			getTargets() const;
		// All the pieces into a one-line string to send over a socket
		std::string				serialiseMsg(void) const;

		// Limited number of setters to finish off the basic message types
		bool					addParams(std::list<std::string> &addme);
		bool					addParams(const std::string &addme);
		bool					insertParam(const std::string &addme);

		// TODO Consolidate these into more ACommand and user-friendly interfaces
		// TODO Rename to reflect their Public visibility
		static Message*	_channelMessage(Message &msg, Channel *chan);
		static Message*	_replyNonNumeric(Message &msg, Channel *chan);
		static Message*	_replyNonNumeric(Message &msg);
		static Message*	_replyThirdParty(Message &msg, User* target);
		// HACK Public to be friend with message origin (user)
		static Message*	_reply(Message &msg, int num_rep);
		static Message*	_reply(Message &msg, int num_rep, Channel *chan);
		static Message*	_reply(Message &msg, int rep_code, User *usr);
		static Message*	_reply(Message &msg, int rep_code, Channel *chan, User *usr);
		static std::list<std::string>	_getParamForNumReply(Message &msg, int rep_code, Channel *chan = 0, User *usr = 0);

};

// TODO How can we make this return nothing when Message is absent/empty?
inline std::ostream&	operator<<(std::ostream &out, const Message &msg)
{
	std::list<std::string>	params;
	std::string				tmp;

	if (!msg.getOrigin())
		std::cout << "Message of unknown origin" << std::endl;
	tmp = msg.getTags();
	if (!tmp.empty())
		out << "Tags: " << tmp << std::endl;
	tmp = msg.getSource();
	if (!tmp.empty())
		out << "Source: " << tmp << std::endl;
	tmp = msg.getCommand();
	if (!tmp.empty())
		out << "Command: " << tmp << std::endl;
	params = msg.getParams();

	if (!params.empty())
	{
		std::list<std::string>::const_iterator  it = params.begin();
		out << "Parameters (" << params.size() << "):";
		while (it != params.end())
		{
			out << *it << ",";
			it++;
		}
		out << std::endl;
	}
//	out << "Parameters: " << msg.getParams() << std::endl;
	return (out);
}
#endif
