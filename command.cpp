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
        User *oldGuest = context->findGuestUserByNickName(args);

        // TODO: change 0.0.0.0 with user ip
        oldGuest->send("ERROR :Closing Link: 0.0.0.0 (Overridden)\n");
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
        ch->broadcast(oss.str());
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
    std::pair<std::string, std::string> p = split(args, ' ');
    if (p.first.empty())
        throw std::invalid_argument(Error::ERR_NORECIPIENT("localhost", user.nickname));
    if (p.second.empty())
        throw std::invalid_argument(Error::ERR_NOTEXTTOSEND("localhost", user.nickname));
    if (p.first[0] == '#' && !context->isChannelExist(p.first))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", user.nickname, p.first));
    if (p.first[0] != '#' && !context->isNickNameRegistred(p.first))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", user.nickname, p.first));
}

void PrivMsgCommand::run(User &user, const std::string &args)
{
    std::pair<std::string, std::string> p = split(args, ' ');

    /**
     * if message start with `:` -> take all the words after `:`
     * else take first word only
     */
    std::string message = ":" + (p.second[0] == ':' ? p.second.substr(1) : getFirstWord(p.second.c_str()));

    std::ostringstream oss;
    std::string channelTagOrNickname = p.first;
    oss << user.getMsgPrefix() << " PRIVMSG " << channelTagOrNickname << " " << message << std::endl;
    message = oss.str();

    if (p.first[0] == '#')
    {
        // sending to channel
        Channel *ch = context->getChannel(channelTagOrNickname);
        if (ch)
            ch->emit(user, message);
    }
    else
    {
        // sending to a specific client
        User *targetUser = context->findRegistredUserByNickname(channelTagOrNickname);
        targetUser->send(message);
    }
}

/**
 * QUIT COMMAND
 */
QuitCommand::QuitCommand(Context *context)
    : CmdHandler(context)
{
}

void QuitCommand::validate(User &user, const std::string &args)
{
    (void)user;
    (void)args;
    // no validation here
}

void QuitCommand::run(User &user, const std::string &args)
{
    (void)args;
    std::cout << "running quit" << std::endl;
    if (user.isRegistred())
    {
        std::ostringstream oss;
        oss << user.getMsgPrefix() << " QUIT :Client Quit\n";
        user.sendToUserChannels(oss.str());
    }

    // TODO: change 127.0.0.1 with user ip
    user.send("ERROR :Closing Link: 127.0.0.1 (Client Quit)\n");
    context->disconnectUser(user.fd);
}
