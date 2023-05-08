#include "command.hpp"

CmdHandler::~CmdHandler() {}

CmdHandler::CmdHandler(Context *context)
    : context(context)
{
}

/**
 * PASS COMMAND
 */

PassCommand::PassCommand(Context *context)
    : CmdHandler(context)
{
}
void PassCommand::validate(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    std::cout << "validate PassCommand " << std::endl;
}

void PassCommand::run(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    std::cout << "run PassCommand " << std::endl;
}

/**
 * USER COMMAND
 */

UserCommand::UserCommand(Context *context)
    : CmdHandler(context)
{
}
void UserCommand::validate(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    std::cout << "validate UserCommand " << std::endl;
}

void UserCommand::run(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    std::cout << "run UserCommand " << std::endl;
}

/**
 * NICK COMMAND
 */

NickCommand::NickCommand(Context *context)
    : CmdHandler(context)
{
}
void NickCommand::validate(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    std::cout << "validate NickCommand " << std::endl;
}

void NickCommand::run(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    std::cout << "run NickCommand " << std::endl;
}
