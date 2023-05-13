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
        throw std::invalid_argument("");
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

    std::ostringstream oss;
    oss << ":" << user.nickname << "!" << user.username << "@localhost NICK :" << newNickname << '\n';

    user.nickname = newNickname;

    if (context->isNickNameRegistred(newNickname))
    {
        std::vector<Channel *> channels = user.channels();
        std::vector<Channel *>::iterator ch_it = channels.begin();
        for (; ch_it != channels.end(); ch_it++)
            // (*ch_it)->emit(user, oss.str());
        user.send(oss.str());
    }
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

    if (tag == "0")
        return;
    if (tag.find(',') != std::string::npos)
    {
        std::cout << "multiple channels" << std::endl;
        std::istringstream iss(tag);
        std::string arg;
        while (std::getline(iss, arg, ','))
        {
            if (arg == "0")
                user.send(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, arg));
            else
            {
                std::string message = "JOIN " + arg;
                user.handleSocket(Command::fromMessage(message));
            }
        }
        throw std::invalid_argument("");
    }
    else if (tag.length() == 0)
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));
    else if (tag[0] != '#')
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, tag));
    if (user.isJoinedChannel(tag))
        throw std::invalid_argument("");
}

void JoinCommand::run(User &user, const std::string &tag)
{
    std::cout << "running join command -> " << tag << std::endl;
    if (tag == "0")
    {
        std::vector<Channel *> channels = user.channels();
        std::vector<Channel *>::iterator ch_it = channels.begin();
        for (; ch_it != channels.end() && (*ch_it); ch_it++)
        {
            std::string message = "PART " + (*ch_it)->getTag();
            user.handleSocket(Command::fromMessage(message));
        }
    }
    else
    {
        context->joinUserToChannel(user, tag);

        std::ostringstream oss;
        oss << user.getMsgPrefix() << " JOIN :" << tag << std::endl;
        context->getChannel(tag)->broadcast(oss.str());

        oss.str("");
        oss.clear();
        oss << ":localhost 353 " << user.nickname << " = " << tag << " :@" << user.nickname << std::endl
            << ":localhost 366 " << user.nickname << " :End of /NAMES list." << std::endl;
        user.send(oss.str());
    }
}

/**
 * PART COMMAND
 */
PartCommand::PartCommand(Context *context)
    : CmdHandler(context)
{
}

void PartCommand::validate(User &user, const std::string &args)
{
    (void)user;
    if (args.empty())
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));
    else if (!context->isChannelExist(args))
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, args));
}

void PartCommand::run(User &user, const std::string &args)
{
    Channel *ch = context->getChannel(args);
    if (ch)
    {
        ch->kickUser(user);
        std::ostringstream oss;
        oss << user.getMsgPrefix() << " PART " << ch->getTag() << '\n';
        user.send(oss.str());
    }
}

/**
 * PRIVMSG COMMAND
 */
PrivMsgCommand::PrivMsgCommand(Context *context)
    : CmdHandler(context)
{
}

void PrivMsgCommand::validate(User &user, const std::string &args)
{
    std::istringstream ss(args);
    std::string token;
    int i = 0;
    while(std::getline(ss, token, ' '))
    {
        if (i == 0)
        {
            // if (token.substr(0,1) != std::string::npos)
            if (token.substr(0, 1) == "#")
            {
                if (!context->isChannelExist(token))
                    throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", token, user.nickname));
                std::cout << "SEND PRIVMSG TO CHANNEL" << std::endl;

            }
            else if(!context->isNickNameRegistred(token))
                throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", token, user.nickname));
        }
        i++;
    }

    
}

void PrivMsgCommand::run(User &user, const std::string &args)
{
    std::istringstream ss(args);
    std::string token, channel;
    std::size_t p;
    Channel *ch;
    int isChannel = 0;
    int colon = 0;
    int i = 0;
    while(std::getline(ss, token, ' '))
    {
        REGISTRED_USERS_MAP::iterator it;
        std::ostringstream oss;
        if (i == 0)
        {
            if (token.substr(0, 1) == "#")
            {
                isChannel = 1;
                channel = token;
                ch = context->getChannel(token);
            }
            else
                it = context->findRegistredUserByNickname(token);           
        }
        else if (i == 1)
        {
            if (token.substr(0, 1) == ":")
            {
                colon = 1;
                p = args.find_first_of(":");
            }
            if (isChannel == 1 && !colon)
            {
                oss << ":" << user.nickname << "!" << user.username << "@localhost" << " PRIVMSG " << channel << " :" << token << '\n';
                ch->emit(user ,oss.str());
            }
            if (isChannel == 1 && colon)
            {
                oss << ":" << user.nickname << "!" << user.username << "@localhost" << " PRIVMSG " << channel << " :" << args.substr(p+1) << '\n';
                ch->emit(user ,oss.str());
            }
            else if(!colon)
            {
                oss << it->second.getMsgPrefix() << " PRIVMSG " << user.nickname << " :" <<token << '\n';
                it->second.send(oss.str());
            }
            else
            {
                oss << it->second.getMsgPrefix() << " PRIVMSG " << user.nickname << " :" << args.substr(p+1) << '\n';
                it->second.send(oss.str());
            }
        }
        i++;
    }
}