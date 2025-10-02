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

// TODO Add getters for any relevant attribute
// TODO Decide if any other information is useful to us here - destination?
// TODO We need something to check that the message is complete and logical
class	Message
{
	private:
		std::string				_tags;		// IF we supported these, they should be stored as multi?MAP of strings
		std::string				_source;	// TODO This might be the wrong format
		std::string				_command;	// Could be an int instead if we convert to the numeric?
		std::list<std::string>	_params;
		User*					_origin;	// Link to who sent this? Allows key info to be retrieved
		std::list<int>			_targets;	// fds where the Message shuold be sent



					Message(void);	// private so not called - no blank messages please
		Message		operator=(const Message &irc);	// NOTE Not sure about assignment to a Message, this could be public

		void		_stepOver(std::istringstream &strm) const;
		void		_parseMessage(std::string text_recvd);
		std::string	_paramToString(std::list<std::string> lst);	// NOTE Helper for making transmittable messages
		std::string	_serialiseMsg(void);	// All the pieces into a one-line string

	public:
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
		User*					getOrigin() const;
		std::list<int>			getTargets() const;


		// Declared as friend so it can access User._origin
		friend	void	Server::_processQueue(void);
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
