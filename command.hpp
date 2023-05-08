#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include "irc_user.hpp"

class Context;

#include "context.hpp"

class Command
{
public:
    std::string cmd;
    std::string args;

    void runCommand(Context *context, User &user) const;
};

class CmdHandler
{
protected:
    std::string args;
    User &user;
    Context *context;

public:
    CmdHandler(Context *context, User &user, const std::string &args);
    virtual void validate() = 0;
    virtual void run() = 0;
};

class PassCommand : public CmdHandler
{
public:
    PassCommand(Context *context, User &user, const std::string &args);
    void validate();
    void run();
};

class NickCommand : public CmdHandler
{
public:
    NickCommand(Context *context, User &user, const std::string &args);
    void validate();
    void run();
};

class UserCommand : public CmdHandler
{
public:
    UserCommand(Context *context, User &user, const std::string &args);
    void validate();
    void run();
};

#endif
