#ifndef USER_HPP
# define USER_HPP

#include <netinet/in.h>		// Needed for sockaddr_in
#include <string>
#include <iostream>
#include <ctime>
#include <list>	// Returned by getWhoReply()

// TODO Decide if any other information is useful to us here
// DONE Add a last seen timestamp (what format?) to allow for timeouts
// TODO Add getter for time in appropriate format (string?)
// TODO Add a "display as source" method giving info to add to Message.source
// .....which commands need that?
// TODO Add _address info to the << display override
// TODO Extract useful info from sockaddr_in *on construction*
// ....to save complications for function callers (me, I am the function caller)
// TODO Add a setMode(modestring) method akin to Channel's
// TODO Support invisible user mode +i
// - store in User
// - use to filter results (might have to be in another Class)
// TODO A public(?) method isVisibleTo(User) that can be used to filter parameters
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
		time_t					last_seen;	// to refer to in case of partial registration
		bool					_isAway;
		bool					_isServerOp;	// +o Has power to shutdown Server, etc
		bool					_isInvisible;	// +i implications for user listings

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
		bool				isRegistered() const;
		sockaddr_in			getAddress() const;		// NOTE This is too low-level to be public IMO
		std::string			getHost() const;
		std::list<std::string>	getWhoReply(void) const;
		std::list<std::string>	getWhoIs(void) const;
		std::string			getFlags(void) const;
		std::string			getUserHostMsg(void) const;

		// Set values
		void				switchVerification();
		void				setNick(std::string nick);
		void				setUser(std::string user);
		void				setReal(std::string rname);
		void				setAway(bool areyou);
		void				updateTime(void);
		bool				setMode(std::string modestr);	// return if changes made (tbc?)
		bool 				_setModeLetter(char mode, bool add, const std::string &param);

		// Channel-related operations
		void				addChannel(const std::string &channel);
		void				removeChannel(const std::string &channel);
};

// NOTE Remember to update this alongside the class members
// TODO Add a last_seen entry
// TODO Add flags output, isAway, Server Op
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
