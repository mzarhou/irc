#include "command.hpp"

CmdHandler::CmdHandler(Context *context, User &user, const std::string &args)
    : context(context), args(args), user(user)
{
}

void Command::runCommand(Context *context, User &user) const
{
    if (cmd == "NICK")
    {
        NickCommand command(context, user, args);
        command.validate();
        command.run();
    }
    else if (cmd == "PASS")
    {
        PassCommand command(context, user, args);
        command.validate();
        command.run();
    }
    else if (cmd == "USER")
    {
        UserCommand command(context, user, args);
        command.validate();
        command.run();
    }
    else
    {
        throw std::invalid_argument("invalid command");
    }
}

/**
 * PASS COMMAND
 */
PassCommand::PassCommand(Context *context, User &user, const std::string &args)
    : CmdHandler(context, user, args)
{
}

void PassCommand::validate()
{
}

void PassCommand::run()
{
}

/**
 * USER COMMAND
 */
UserCommand::UserCommand(Context *context, User &user, const std::string &args)
    : CmdHandler(context, user, args)
{
}

void UserCommand::validate()
{
}

void UserCommand::run()
{
}

/**
 * NICK COMMAND
 */
NickCommand::NickCommand(Context *context, User &user, const std::string &args)
    : CmdHandler(context, user, args)
{
}

void NickCommand::validate()
{
}

void NickCommand::run()
{
}
