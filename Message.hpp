#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <list>

// TODO Add getters for any relevant attribute
// TODO Decide if any other information is useful to us here
class	Message
{
	private:
		std::string	_tags;		// IF we supported these, they should be stored as multi?MAP of strings
		std::string	_source;	// TODO This might be the wrong format
		std::string	_command;	// Could be an int instead if we convert to the numeric?
		std::list<std::string>	_params;


					Message(void);	// private so not called - no blank messages please
					Message(const Message &irc);	// No good reason to allow copy construction of the server
		Message		operator=(const Message &irc);	// No assignment should be possible either

		int			acceptClient();

	public:
					Message(std::string raw_text);
					~Message(void);
};
#endif
