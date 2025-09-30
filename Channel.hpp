#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <set>
#include <map>

class User;

class Channel
{
	private:
		std::string					_name;
		std::string					_topic;
		std::set<int>				_members;		// User FDs
		std::set<int>				_operators;		// User FDs with operator rights
		std::set<std::string>		_invitedNicks;	// Invited nicks (by name)
		bool						_topicProtected;	// +t mode
		bool						_inviteOnly;		// +i mode
		std::string					_password;		// +k mode
		int							_userLimit;		// +l mode
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
		const std::set<int>&		getMembers(void) const;
		const std::set<int>&		getOperators(void) const;
		const std::set<std::string>& getInvitedNicks(void) const;
		bool						isTopicProtected(void) const;
		bool						isInviteOnly(void) const;
		bool						hasPassword(void) const;
		const std::string&			getPassword(void) const;
		int							getUserLimit(void) const;
		size_t						getMemberCount(void) const;

		// Setters
		void						setTopic(const std::string &topic);
		void						setTopicProtected(bool topicProtected);
		void						setInviteOnly(bool inviteOnly);
		void						setPassword(const std::string &password);
		void						clearPassword(void);
		void						setUserLimit(int limit);

		// Member management
		bool						addMember(int userFd);
		bool						removeMember(int userFd);
		bool						isMember(int userFd) const;
		bool						isOperator(int userFd) const;
		bool						addOperator(int userFd);
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
