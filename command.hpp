#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "irc_user.hpp"

class Context;

#include "context.hpp"

struct Command
{
    std::string originalName;
    std::string name;
    std::string args;
};

class CmdHandler
{
protected:
    Context *context;

public:
    CmdHandler(Context *context);
    virtual ~CmdHandler();
    virtual void validate(User &user, const std::string &args) = 0;
    virtual void run(User &user, const std::string &args) = 0;
};

class PassCommand : public CmdHandler
{
public:
    PassCommand(Context *context);
    void validate(User &user, const std::string &args);
    void run(User &user, const std::string &args);
};

class NickCommand : public CmdHandler
{
public:
    NickCommand(Context *context);
    void validate(User &user, const std::string &args);
    void run(User &user, const std::string &args);
};

class UserCommand : public CmdHandler
{
public:
    UserCommand(Context *context);
    void validate(User &user, const std::string &args);
    void run(User &user, const std::string &args);
};

class ListCommand : public CmdHandler
{
public:
    ListCommand(Context *context);
    void validate(User &user, const std::string &args);
    void run(User &user, const std::string &args);
};

#endif
