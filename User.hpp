#ifndef USER_HPP
# define USER_HPP

#include <netinet/in.h>		// Needed for sockaddr_in
#include <string>
#include <iostream>

// TODO Decide if any other information is useful to us here
// TODO Add a last seen timestamp (what format?) to allow for timeouts
// TODO Decide how to store MODEs
// TODO Add more constructors as appropriate
// TODO Decide how to store server info
// TODO Add a "display as source" method giving info to add to Message.source
// .....which commands need that?
// TODO Add _address info to the << display override
// TODO Extract useful info from sockaddr_in *on construction*
// ....to save complications for function callers (me, I am the function caller)
class	User
{
	private:
		int						_fd;
		std::string				_nick;
		std::string				_uname;
		std::string				_rname;
		bool					_gavepass;
		sockaddr_in				_address;	// has sin_port and sin_addr
		std::string				_host;		// readable socket address for use in messages
		// int _mode;	// How do we store / manage this?
		// time_t	last_seen;	// to refer to in case of partial registration

		User		operator=(const User &irc);	// NOTE Not sure about assignment to a User, this could be public

	public:
		User(void);	// Which other versions of this are needed?
		User(int fd);	// Use fd to get more info
		User(const User &irc);	// Copying a User seems reasonable to allow
		~User(void);

		static User*		makeUser(int fd);

		int					getFD() const;
		std::string			getNick() const;
		std::string			getUser() const;
		std::string			getReal() const;
		bool				isVerified() const;
		sockaddr_in			getAddress() const;		// NOTE This is too low-level to be public IMO
		std::string			getHost() const;
		void				switchVerification();
};

// NOTE Remember to update this alongside the class members
inline std::ostream&	operator<<(std::ostream &out, const User &usr)
{
	std::string				tmp;

	if (usr.isVerified())
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
	else
		out << "Unverified user" << std::endl;
	std::cout << "Socket FD: " << usr.getFD() << std::endl;
	tmp = usr.getHost();
	if (!tmp.empty())
		out << "Host: " << tmp << std::endl;
	return (out);
}
#endif
