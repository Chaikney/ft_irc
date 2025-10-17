#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <set>
#include <map>
#include <list>	// returns FDs for use in Messages
#include <ctime>

class User;

class Channel
{
	private:
		std::string					_name;
		std::string					_topic;
		time_t						_topicTime;
		std::string					_topicSetBy;
		std::set<User *>			_members;		// User FDs
		// TODO Seems to me that these should store User* not just file descriptors....
		std::set<int>				_operators;		// User FDs with operator rights
		std::set<std::string>		_invitedNicks;	// Invited nicks (by name)
		bool						_topicProtected;	// +t mode
		bool						_inviteOnly;		// +i mode
		std::string					_password;		// +k mode
		int							_userLimit;		// +l mode
		// NOTE All these work with NICKs so string is the correct format
		std::set<std::string>		_banList;		// +b mode
		std::set<std::string>		_exceptionList;	// +e mode
		std::set<std::string>		_inviteList;	// +I mode

		Channel(void);
		Channel(const Channel &other);
		Channel& operator=(const Channel &other);

	public:
		explicit Channel(const std::string &name);
		~Channel(void);

		// Getters
		const std::string&			getName(void) const;
		const std::string&			getTopic(void) const;
		const std::set<User *>&		getMembers(void) const;
		const std::set<int>&		getOperators(void) const;
		const std::set<std::string>& getInvitedNicks(void) const;
		bool						isTopicProtected(void) const;
		bool						isInviteOnly(void) const;
		bool						hasPassword(void) const;
		const std::string&			getPassword(void) const;
		int							getUserLimit(void) const;
		size_t						getMemberCount(void) const;
		std::string		getMemberCountText(void) const;
		std::list<int>		getBroadcastFDs(void) const;	// for use with PRIVMSG, NOTICE, etc
		std::list<int>		getBroadcastFDs(User *usr) const;	// as above but excluding one User
		std::list<std::string>	getListInfo(void) const;	// for use in RPL_LIST

		// Setters
//		void						setTopic(const std::string &topic);
		void						setTopic(const std::string &topic, const std::string &set_by);
		void						setTopicProtected(bool topicProtected);
		void						setInviteOnly(bool inviteOnly);
		void						setPassword(const std::string &password);
		void						clearPassword(void);
		void						setUserLimit(int limit);

		// Member management
		bool						addMember(User *usr);
		//bool						removeMember(int userFd);
		bool						removeMember(User *usr);
		bool						isMember(User *usr) const;
		//bool						isMember(int userFd) const;
		bool						isOperator(int userFd) const;
		bool						addOperator(User *usr);
		bool						removeOperator(int userFd);

		// Invite management
		bool						addInvite(const std::string &nick);
		bool						removeInvite(const std::string &nick);
		bool						isInvited(const std::string &nick) const;

		// Ban management
		bool						addBan(const std::string &mask);
		bool						removeBan(const std::string &mask);
		bool						isBanned(const std::string &mask) const;

		// Mode management
		std::string					getModeString(void) const;
		bool						setMode(char mode, bool add, const std::string &param = "");

		// Utility
		bool						isEmpty(void) const;
		void						clear(void);
};

#endif
