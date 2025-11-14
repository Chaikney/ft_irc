#ifndef USER_HPP
# define USER_HPP

#include <netinet/in.h>		// Needed for sockaddr_in
#include <string>
#include <iostream>
#include <ctime>
#include <list>	// Returned by getWhoReply()
#include <set>	// Stores pointers to all Channels the User is in

class	Channel;
#include "Server.hpp"	// access to the SERVERNAME string

// DONE Add a last seen timestamp (what format?) to allow for timeouts
// IDEA Add getter for time in appropriate format (string?)
// IDEA Add _address info to the << display override
// IDEA Extract useful info from sockaddr_in *on construction*
// ....to save complications for function callers (me, I am the function caller)
// IDEA Add a setMode(modestring) method akin to Channel's
// IDEA Support invisible user mode +i
// - store in User
// - use to filter results (might have to be in another Class)
// IDEA A public(?) method isVisibleTo(User) that can be used to filter parameters
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
		std::string				_serverName;	// Server that the user is connected to; always our server (needed for WHOREPLY)
		time_t					last_seen;	// to refer to in case of partial registration
		bool					_isAway;
		std::string				_awayMsg;
		bool					_isServerOp;	// +o Has power to shutdown Server, etc
		std::set<Channel *>		_memberships;

		User		operator=(const User &irc);	// NOTE Not sure about assignment to a User, this could be public

		bool 				_setModeLetter(char mode, bool add);
	public:
		User(void);	// Which other versions of this are needed?
		User(int fd);	// Use fd to get more info
		User(const User &irc);	// Copying a User seems reasonable to allow
		~User(void);


		// Various getters, some for specific Numeric replies
		int				getFD() const;
		std::string			getNick() const;
		std::string			getUser() const;
		std::string			getReal() const;
		bool				isVerified() const;
		bool				isRegistered() const;
		bool				isAway() const;
		sockaddr_in			getAddress() const;		// NOTE This is too low-level to be public IMO
		std::string			getHost() const;
		std::string			getServerName(void) const;
		std::list<std::string>		getWhoReply(void) const;
		std::list<std::string>		getWhoIs(void) const;
		std::string			getFlags(void) const;
		std::string			getUserHostMsg(void) const;
		std::string			getModes(void) const;
		std::set<Channel *>		getMemberships(void) const;
		std::string			getAwayMsg(void) const;

		// Set values
		void				switchVerification();
		void				setNick(std::string nick);
		void				setUser(std::string user);
		void				setReal(std::string rname);
		void				setAway(bool areyou);
		void				setAwayMsg(std::string str);
		void				updateTime(void);
		bool				setMode(std::string modestr);	// return if changes made (tbc?)

		// Channel-related operations
		void				addChannel(Channel* chan);
		void				removeChannel(Channel* chan);

		// User-related methods that don't need an existing instance
		static User*			makeUser(int fd);
		static bool			normaliseNick(std::string *nick);

		// Comparison overloads
		friend bool	operator==(const User &lhs, const User &rhs);
		friend bool	operator!=(const User &lhs, const User &rhs);
};

// IDEA Make the USER equality comparison more robust
inline bool	operator==(const User &lhs, const User &rhs)
{
	return (lhs._nick == rhs._nick);
}

inline bool	operator!=(const User &lhs, const User &rhs)
{
	return (lhs._nick != rhs._nick);
}

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
