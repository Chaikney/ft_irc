#ifndef USER_HPP
# define USER_HPP

#include <string>
#include <iostream>
//#include <sys/socket.h>		// Needed if we use sockaddr_in

// TODO Decide if any other information is useful to us here
// TODO Add a last seen timestamp (what format?) to allow for timeouts
// TODO Decide how to store MODEs
// TODO Add more constructors as appropriate
// TODO Decide how to store server info
// TODO Add a "display as source" method giving info to add to Message.source
// .....which commands need that?
class	User
{
	private:
		std::string				_nick;
		std::string				_uname;
		std::string				_rname;
		bool					_gavepass;
		// sockaddr_in			_address;	// has sin_port and sin_addr
		// int _mode;	// How do we store / manage this?
		// time_t	last_seen;	// to refer to in case of partial registration

		User		operator=(const User &irc);	// NOTE Not sure about assignment to a User, this could be public

	public:
		User(void);	// Which other versions of this are needed?
		User(const User &irc);	// Copying a User seems reasonable to allow
		~User(void);

		static User*		makeUser(void);

		std::string			getNick() const;
		std::string			getUser() const;
		std::string			getReal() const;
		bool				isVerified() const;
};

// NOTE Remember to update this alongside the class members
inline std::ostream&	operator<<(std::ostream &out, const User &usr)
{
	std::string				tmp;

	if (!usr.isVerified())
		out << "Unverified user" << std::endl;
	else
	{
		tmp = usr.getNick();
		if (!tmp.empty())
			out << "Nick: " << tmp << std::endl;
		tmp = usr.getUser();
		if (!tmp.empty())
			out << "User " << tmp << std::endl;
		tmp = usr.getReal();
		if (!tmp.empty())
			out << "Realname: " << tmp << std::endl;
	}
	return (out);
}
#endif
