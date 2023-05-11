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
    std::cout << "validate PassCommand " << std::endl;
    if (user.isRegistred())
        throw std::invalid_argument(":localhost 462 * :You are already registred and cannot handshake again\n");
    else if (args.empty())
        throw std::invalid_argument(":localhost 461 * PASS :Not enough parameters\n");
    else if (args.compare(context->getServerpassw()) != 0)
        throw std::invalid_argument(":localhost 464 * PASS :Password incorrect\n");
}

void PassCommand::run(User &user, const std::string &args)
{
    user.password = args;
    std::cout << "run PassCommand with passw: " << args << std::endl;
}

/**
 * USER COMMAND
 */
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
    while (std::getline(ss, token, ' '))
    {
        if (i == 0)
            tmp = token;
        if (i == 1)
        {
            if (token.compare("0") != 0)
                return 0;
        }
        if (i == 2)
        {
            if (token.compare("*") != 0)
                return 0;
        }
        if (i == 3)
        {
            if (token.compare(tmp) == 0)
                return 0;
        }
        i++;
    }
    if (i != 4)
        return 0;
    return 1;
}

void UserCommand::validate(User &user, const std::string &args)
{
    if (user.isRegistred())
        throw std::invalid_argument(":localhost 462 * :You are already registred and cannot handshake again\n");
    if (user.password.empty())
        throw std::invalid_argument(":localhost * :No password given\n");
    if (args.empty() || !check_args(args))
        throw std::invalid_argument(":localhost 461 * USER :Not enough parameters\n");
}

void UserCommand::run(User &user, const std::string &args)
{
    std::cout << "run UserCommand " << std::endl;
    std::istringstream ss(args);
    std::string token, tmp;
    int i = 0;
    while (std::getline(ss, token, ' '))
    {
        if (i == 0)
            user.username = "~" + token;
        if (i == 1)
            // add rule
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

void NickCommand::validate(User &user, const std::string &args)
{
    // TODO: check nickname syntax
    if (user.password.empty())
        throw std::invalid_argument(":localhost * :No password given\n");
    if (args.empty())
        throw std::invalid_argument(":localhost 431 * :No nickname given\n");
    if (user.nickname == args)
        return;
    if (context->isNickNameRegistred(args))
    {
        std::ostringstream oss;
        oss << ":localhost 433 * " << args << " :Nickname is already in use.\n";
        throw std::invalid_argument(oss.str());
    }

    /**
     * disconnect old guest user with similar nickname if exists
     */
    if (context->isNickNameGuest(args))
    {
        User &oldGuest = context->findGuestUserByNickName(args)->second;
        oldGuest.send("ERROR :Closing Link: 0.0.0.0 (Overridden)\n");
        context->disconnectUser(args);
    }
}

void NickCommand::run(User &user, const std::string &newNickname)
{
    std::cout << "run NickCommand " << std::endl;

    if (user.nickname == newNickname)
    {
        return;
    }

    std::ostringstream oss;
    oss << ":" << user.nickname << "!" << user.username << "@localhost NICK :" << newNickname << '\n';

    user.nickname = newNickname;

    if (context->isNickNameRegistred(newNickname))
        user.send(oss.str());
}

/**
 * LIST COMMAND
 */
ListCommand::ListCommand(Context *context)
    : CmdHandler(context)
{
}

void ListCommand::validate(User &user, const std::string &args)
{
    (void)user;
    (void)args;
}

void ListCommand::run(User &user, const std::string &args)
{
    (void)user;
    (void)args;
}

/**
 * JOIN COMMAND
 */
JoinCommand::JoinCommand(Context *context)
    : CmdHandler(context)
{
}

void JoinCommand::validate(User &user, const std::string &tag)
{
    // TODO: modes
    (void)user;

    if (tag.find(',') != std::string::npos)
    {
        std::cout << "multiple channels" << std::endl;
        std::istringstream iss(tag);
        std::string arg;
        while (std::getline(iss, arg, ','))
        {
            std::string message = "JOIN " + arg;
            user.handleSocket(Command::fromMessage(message));
        }
        throw std::invalid_argument("");
    }
    if (tag.length() == 0)
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));
    if (tag[0] != '#')
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, tag));
}

void JoinCommand::run(User &user, const std::string &tag)
{
    std::cout << "running join command -> " << tag << std::endl;
    context->joinUserToChannel(user, tag);
}
