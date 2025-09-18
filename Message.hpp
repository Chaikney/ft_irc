#ifndef MESSAGE_HPP
# define MESSAGE_HPP

#include <string>
#include <list>

// TODO Add getters for any relevant attribute
// TODO Decide if any other information is useful to us here
class	Message
{
	private:
		std::string				_tags;		// IF we supported these, they should be stored as multi?MAP of strings
		std::string				_source;	// TODO This might be the wrong format
		std::string				_command;	// Could be an int instead if we convert to the numeric?
		std::list<std::string>	_params;


					Message(void);	// private so not called - no blank messages please
		Message		operator=(const Message &irc);	// NOTE Not sure about assignment to a Message, this could be public

	public:
					Message(std::string raw_text);
					Message(const Message &irc);	// Copying a Message seems reasonable to allow
					~Message(void);

		std::string				getCommand() const;
		std::list<std::string>	getParams() const;
};
#endif
