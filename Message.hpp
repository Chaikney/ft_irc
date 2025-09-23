#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <list>
#include <iostream>

// Static member variable. All messages of the same max length bytes
const int	MSG_LEN = 512;

// TODO Add getters for any relevant attribute
// TODO Decide if any other information is useful to us here
// TODO We need something to check that the message is complete and logical
class	Message
{
	private:
		std::string				_tags;		// IF we supported these, they should be stored as multi?MAP of strings
		std::string				_source;	// TODO This might be the wrong format
		std::string				_command;	// Could be an int instead if we convert to the numeric?
		std::list<std::string>	_params;


					Message(void);	// private so not called - no blank messages please
		Message		operator=(const Message &irc);	// NOTE Not sure about assignment to a Message, this could be public

		void		_stepOver(std::istringstream &strm) const;

	public:
					Message(std::string raw_text);
					Message(const Message &irc);	// Copying a Message seems reasonable to allow
					~Message(void);

		static Message*				makeMessage(std::string &str);
		std::string				getTags() const;
		std::string				getSource() const;
		std::string				getCommand() const;
		std::list<std::string>	getParams() const;
};

inline std::ostream&	operator<<(std::ostream &out, const Message &msg)
{
	std::list<std::string>	params;
	std::string				tmp;

	tmp = msg.getTags();
	if (!tmp.empty())
		out << "Tags: " << msg.getTags() << std::endl;
	tmp = msg.getSource();
	if (!tmp.empty())
		out << "Source: " << msg.getSource() << std::endl;
	tmp = msg.getCommand();
	if (!tmp.empty())
		out << "Command: " << msg.getCommand() << std::endl;
	params = msg.getParams();

	if (!params.empty())
	{
		std::list<std::string>::const_iterator  it = params.begin();
		out << "Parameters: ";
		while (it != params.end())
		{
			out << *it << ", ";
			it++;
		}
		out << std::endl;
	}
//	out << "Parameters: " << msg.getParams() << std::endl;
	return (out);
}
#endif
