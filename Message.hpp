#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <list>
#include <iostream>
#include <map>

class	User;
// NOTE use include not forward definition so the "friend" keyword works
// class Server;
#include "Server.hpp"

// Static member variable. All messages of the same max length bytes
const int	MSG_LEN = 512;

// IRC message format: [@tags] [:]source command [params] [:trailing]
class	Message
{
	private:
		std::map<std::string, std::string>	_tags;		// IRCv3 tags
		std::string							_source;	// message source (server or user)
		std::string							_command;	// command or numeric
		std::list<std::string>				_params;	// parameters
		std::string							_trailing;	// trailing parameter (after :)
		User*								_origin;	// Link to who sent this? Allows key  info to be retrieved
		bool								_isNumeric;	// true if command is a numeric reply


					Message(void);	// private so not called - no blank messages please
		Message		operator=(const Message &irc);	// NOTE Not sure about assignment to a Message, this could be public

		void		_stepOver(std::istringstream &strm) const;
		void		_parseMessage(std::string text_recvd);
		void		_parseTags(const std::string &tagString);

	public:
					Message(std::string raw_text);
					Message(std::string text_recvd, User *usr);
					Message(const Message &irc);	// Copying a Message seems reasonable to allow
					~Message(void);

		static Message*			makeMessage(std::string &str);
		static Message*			makeMessage(std::string &str, User *origin);
		
		// Getters
		const std::map<std::string, std::string>& getTags() const;
		std::string				getTag(const std::string &key) const;
		std::string				getSource() const;
		std::string				getCommand() const;
		std::list<std::string>	getParams() const;
		std::string				getTrailing() const;
		User*					getOrigin() const;
		bool					isNumeric() const;
		
		// Setters
		void					setSource(const std::string &source);
		void					setCommand(const std::string &command);
		void					addParam(const std::string &param);
		void					setTrailing(const std::string &trailing);
		
		// Utility
		std::string				toString() const;	// Convert back to IRC format
		bool					isValid() const;	// Check if message is valid


		// Declare this as friend so it can access _origin
		// ...maybe only _processQueue needed?
		friend	void	Server::_processQueue(void);
};

// TODO How can we make this return nothing when Message is absent/empty?
inline std::ostream&	operator<<(std::ostream &out, const Message &msg)
{
	std::list<std::string>	params;
	std::string				tmp;

	if (!msg.getOrigin())
		std::cout << "Message of unknown origin" << std::endl;
	// Tags are now a map, so we'll skip displaying them in the simple output
	// tmp = msg.getTags();
	// if (!tmp.empty())
	//	out << "Tags: " << tmp << std::endl;
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
