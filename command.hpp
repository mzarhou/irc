#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "irc_user.hpp"
#include "errors.hpp"

class Context;

#include "context.hpp"

struct Command
{
    std::string originalName;
    std::string name;
    std::string args;

    static Command fromMessage(std::string &message)
    {
        Command cmd;

        cmd.originalName = getFirstWord(message.c_str());

        message = trim(message);
        message = upperFirstWord(message.c_str());
        std::istringstream ss(message);
        size_t npos = message.find_first_of(" \t\n\r\v\f");
        if (npos == std::string::npos)
            return (Command){cmd.originalName, message, ""};
        cmd.name = message.substr(0, npos);
        cmd.args = trim(message.substr(npos));
        return cmd;
    }
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

class JoinCommand : public CmdHandler
{
public:
    JoinCommand(Context *context);
    void validate(User &user, const std::string &args);
    void run(User &user, const std::string &args);
};

class PartCommand : public CmdHandler
{
public:
    PartCommand(Context *context);
    void validate(User &user, const std::string &args);
    void run(User &user, const std::string &args);
};

#endif
