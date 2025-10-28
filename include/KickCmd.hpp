#ifndef KICKCMD_HPP
# define KICKCMD_HPP

#include "ACommand.hpp"
#include <string>
#include <list>

class KickCmd : public ACommand {
public:
    KickCmd(Server *srv, Message &msg);
    virtual ~KickCmd();

    virtual void executeCmd(void);
};

#endif