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
int PassCommand::validate(User &user, const std::string &args)
{
    // (void)user;
    // (void)args;
    // int bytes_sent;
    std::cout << "validate PassCommand " << std::endl;
    if (args.empty())
    {
        user.context->sendClientMsg(user.context->last_connected, ":localhost 461 * PASS :Not enough parameters\n");
        return 0;
    }
    else if (args.compare(user.context->getServerpassw()) != 0)
    {
        user.context->sendClientMsg(user.context->last_connected, ":localhost 464 * PASS :Password incorrect\n");
        return 0;
    }
    return 1;
}

void PassCommand::run(User &user, const std::string &args)
{
    // (void)user;
    // (void)args;
    user.password = args;
    std::cout << "run PassCommand " << std::endl;
}

UserCommand::UserCommand(Context *context)
    : CmdHandler(context)
{
}

int check_args(std::string args)
{
    if (args.empty())
        return 0;
    std::istringstream ss(args);
    std::string token, tmp;
    int i = 0;
    while(std::getline(ss, token, ' ')) {
        if (i == 0)
            tmp = token;
        if (i == 1)
        {
            if(token.compare("0") != 0)
                return 0;
        }
        if (i == 2)
        {
            if(token.compare("*") != 0)
                return 0;
        }
        if (i == 3)
        {
            if(token.compare(tmp) == 0)
                return 0;
        }
        i++;
    }
    if (i != 4)
        return 0;
    return 1;
}

int UserCommand::validate(User &user, const std::string &args)
{
    // (void)user;
    // (void)args;
    if (user.password.empty())
    {
        user.context->sendClientMsg(user.context->last_connected, ":localhost * :No password given\n");
        return 0;
    }
    std::cout << args << std::endl;
    if (!args.empty() && check_args(args))
    {
        std::cout << "validate UserCommand " << std::endl;
        return 1;
    }
    else
        user.context->sendClientMsg(user.context->last_connected, ":localhost 461 * USER :Not enough parameters\n");
    return 0;
}

void UserCommand::run(User &user, const std::string &args)
{
    // (void)user;
    // (void)args;
    std::cout << "run UserCommand " << std::endl;
    std::istringstream ss(args);
    std::string token, tmp;
    int i = 0;
    while(std::getline(ss, token, ' ')) {
        if (i == 0)
            user.username = token;
        if (i == 1)
            //add rule
        if (i == 3)
            user.realname = token;
        i++;
    }
}

/**
 * NICK COMMAND
 */

NickCommand::NickCommand(Context *context)
    : CmdHandler(context)
{
}
int NickCommand::validate(User &user, const std::string &args)
{
    // (void)user;
    // (void)args;
    if (user.password.empty())
    {
        user.context->sendClientMsg(user.context->last_connected, ":localhost * :No password given\n");
        return 0;
    }
    if (args.empty())
    {
        user.context->sendClientMsg(user.context->last_connected, ":localhost 431 * :No nickname given\n");
        return 0;
    }
    else
    {
        std::cout << "validate NickCommand " << std::endl;
        return 1;
    }
}

void NickCommand::run(User &user, const std::string &args)
{
    // (void)user;
    // (void)args;
    std::cout << "run NickCommand " << std::endl;
    if (!args.empty())
    {
        user.nickname = args;
    }
}