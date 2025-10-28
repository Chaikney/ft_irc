#include "KickCmd.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include "User.hpp"

KickCmd::KickCmd(Server *srv, Message &msg) : ACommand(srv, msg) {}

KickCmd::~KickCmd() {}

// KICK <channel> <user> [<comment>]
// TODO Remove repetitive parts like checking the channel name, that should be in Channel::_findChannel
// TODO isOperator() probably is based on NICK or USER not a fd? What happens if they reconnect?
// TODO Unified message creation / sending not the hardcoded parameters
// TODO Adapt to handle multiple Users getting kicked from the one channel
// (Although: "Servers MAY limit the number of target users per KICK command via the TARGMAX parameter
// of RPL_ISUPPORT, and silently drop targets if the number of targets exceeds
// the limit.)"
void KickCmd::executeCmd(void)
{
    User *usr = _msg.getOrigin();

    std::list<std::string> params = _msg.getParams();
    if (params.size() < 2)
    {
        _responses.push(Message::_reply(_msg, ERR_NEEDMOREPARAMS));
        return;
    }
    std::string chan = params.front(); params.pop_front();
    std::string nick = params.front(); params.pop_front();
    std::string reason = "";
    if (!params.empty())
        reason = params.front();

    if (Channel::normaliseChanName(&chan) == false)
    {
        _responses.push(Message::_reply(_msg, ERR_BADCHANMASK));
        return;
    }
    Channel *channel = _srv->_findChannel(chan);
    if (!channel)
    {
        _responses.push(Message::_reply(_msg, ERR_NOSUCHCHANNEL));
        return;
    }
    if (!channel->isOperator(usr->getFD()))
    {
        _responses.push(Message::_reply(_msg, ERR_CHANOPRIVSNEEDED));
        return;
    }
    User *target = _srv->_findUserByNick(nick);
    if (!target)
    {
        _responses.push(Message::_reply(_msg, ERR_NOSUCHNICK));
        return;
    }
    if (!channel->isMember(target))
    {
        _responses.push(Message::_reply(_msg, ERR_USERNOTINCHANNEL));
        return;
    }
    channel->removeMember(target);
    target->removeChannel(channel);
    if (reason.empty()) reason = "Kicked";
    _responses.push(Message::_channelMessage(_msg, channel));
    Message *direct = Message::_replyNonNumeric(_msg);
	// FIXME hardcoded notification parameters in KICK
    direct->addParams(std::string(":server KICK " + chan + " " + nick + " :" + reason));
    _responses.push(direct);
}
