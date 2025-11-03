#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <set>
#include <list>	// returns FDs for use in Messages
#include <ctime>

class User;

// TODO Support no External messages ban on sending (e.g. in getBraodcastFDs? or elsewhere?)
class Channel
{
	private:
		std::string			_name;
		time_t				_creationTime;
		std::string			_topic;
		time_t				_topicTime;
		std::string			_topicSetBy;
		std::set<User *>		_members;		// Users in the channel
		// TODO Seems to me that these should store User* not just file descriptors....
		std::set<User *>		_operators;		// User FDs with operator rights
		std::set<std::string>		_invitedNicks;	// Invited nicks (by name)
		bool				_topicProtected;	// +t mode
		bool				_noExtMsg;			// +n no external messages can be sent to the channel (this is kind of implicit in how we have coded it though)
		bool				_inviteOnly;		// +i mode
		std::string			_password;		// +k mode
		int				_userLimit;		// +l mode
		// NOTE All these work with NICKs so string is the correct format
		std::set<std::string>		_banList;	// +b mode
		std::set<std::string>		_exceptionList;	// +e mode
		std::set<std::string>		_inviteList;	// +I mode

		Channel(void);
		Channel(const Channel &other);
		Channel& operator=(const Channel &other);

	public:
		// TODO Explain why we have used the explicit keyword here
		explicit Channel(const std::string &name);
		~Channel(void);

		// Getters
		const std::string&			getName(void) const;
		const std::string&			getTopic(void) const;
		const std::string			getCreationTime(void) const;
		const std::string&			getTopicSetter(void) const;
		const std::string			getTopicTime(void) const;	// NOTE Not returning a reference because using a local variable
		const std::set<User *>&		getMembers(void) const;
		const std::set<User *>&		getOperators(void) const;
		const std::set<std::string>& getInvitedNicks(void) const;
		const std::set<std::string>& getBannedNicks(void) const;
		bool						isTopicProtected(void) const;
		bool						isInviteOnly(void) const;
		bool						hasPassword(void) const;
		const std::string&			getPassword(void) const;
		int			getUserLimit(void) const;
		std::string		getUserLimitText(void) const;
		size_t						getMemberCount(void) const;
		std::string		getMemberCountText(void) const;
		std::list<int>		getBroadcastFDs(void) const;	// for use with PRIVMSG, NOTICE, etc
		std::list<int>		getBroadcastFDs(User *usr) const;	// as above but excluding one User
		std::list<std::string>	getListInfo(void) const;	// for use in RPL_LIST
		std::list<std::string>	getNameReply(void) const;	// for use in RPL_NAMREPLY
		User*		findMemberByNick(std::string target) const;

		// Setters
		// TODO Need an "add banned" method
//		void						setTopic(const std::string &topic);
		void						setTopic(const std::string &topic, const std::string &set_by);
		void						setTopicProtected(bool topicProtected);
		void						setInviteOnly(bool inviteOnly);
		void						setPassword(const std::string &password);
		void						clearPassword(void);
		void						setUserLimit(int limit);

		// Member management
		bool						addMember(User *usr);
		bool						removeMember(User *usr);
		bool						isMember(User *usr) const;
		bool						isOperator(User *usr) const;
		bool						addOperator(User *usr);
		bool						removeOperator(User *usr);

		// Invite management
		bool						addInvite(const std::string &nick);
		bool						removeInvite(const std::string &nick);
		bool						isInvited(const std::string &nick) const;

		// Ban management
		bool						addBan(const std::string &mask);
		bool						removeBan(const std::string &mask);
		bool						isBanned(const std::string &mask) const;

		// Mode management
		// TODO Make setMode an internal thing and have an exeternal piece that takes the whole modestring
		std::string					getModeString(void) const;
		bool			setMode(char mode, bool add, const std::string &param = "");
		bool			setMode(std::string modestring, std::string modearg) ;

		// Utility
		bool						isEmpty(void) const;
		void						clear(void);	// TODO Give this a less-ambiguous name
		static bool	normaliseChanName(std::string *chan);

		bool			checkPassword(std::string const &key) const;
};

#endif
