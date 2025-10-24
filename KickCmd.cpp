#include "KickCmd.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "ReplyEnums.hpp"
#include <iostream>

KickCmd::KickCmd(Server *srv, Message &msg) : ACommand(srv, msg) {}

KickCmd::~KickCmd() {}

void KickCmd::executeCmd(void)
{
    User *usr = _msg.getOrigin();
    std::cout << "[KICK] Comando recibido de fd " << (usr ? usr->getFD() : -1) << std::endl;

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

    if (_srv->normaliseChanName(&chan) == false)
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
    target->removeChannel(chan);
    if (reason.empty()) reason = "Kicked";
    _responses.push(Message::_channelMessage(_msg, channel));
    Message *direct = Message::_replyNonNumeric(_msg);
    direct->addParams(std::string(":server KICK " + chan + " " + nick + " :" + reason));
    _responses.push(direct);
}