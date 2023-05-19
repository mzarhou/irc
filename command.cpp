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
        user.sendToUserChannels(oss.str());
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

void JoinCommand::validate(User &user, const std::string &args)
{
    if (args == "0")
        return;
    std::pair<std::string, std::string> p = split(args, ' ');
    std::string tag = p.first;
    std::string key = p.second;
    if (tag.find(',') != std::string::npos)
    {
        std::cout << "multiple channels" << std::endl;
        std::istringstream iss(tag);
        std::queue<std::string> tags = splitChunks(tag, ',');
        std::queue<std::string> keys = splitChunks(key, ',');
        while (!tags.empty())
        {
            tag = tags.front();
            tags.pop();
            key = "";

            if (!keys.empty())
            {
                key = keys.front();
                keys.pop();
            }

            if (tag == "0")
                user.send(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, tag));
            else
            {
                std::string message = "JOIN " + tag + " " + key;
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
    user.canJoinChannel(user, tag, key);
}

void JoinCommand::run(User &user, const std::string &args)
{

    std::pair<std::string, std::string> p = split(args, ' ');
    std::string tag = p.first;

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

        Channel *ch = context->getChannel(tag);

        {
            std::ostringstream oss;
            oss << user.getMsgPrefix() << " JOIN :" << tag << std::endl;
            ch->broadcast(oss.str());
        }

        std::ostringstream oss;
        if (user.isChannelOp(ch->getTag()))
            oss << ":localhost MODE " << ch->getTag() << " " << ch->getModes(user) << std::endl;
        oss << ":localhost 353 " << user.nickname << " = " << tag << " :" << ch->getUsersStr() << std::endl
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
 * MODE COMMAND
 */
ModeCommand::ModeCommand(Context *context)
    : CmdHandler(context)
{
}

std::queue<std::string> ModeCommand::parseModes(const std::string &modesStr)
{
    char sign = '+';
    std::queue<std::string> queue;

    for (size_t i = 0; i < modesStr.length(); i++)
    {
        char c = modesStr[i];
        if (c == '+' || c == '-')
        {
            sign = c;
            continue;
        }
        queue.push(std::string() + sign + c);
    }
    return queue;
}

std::queue<std::string> ModeCommand::parseModesArgs(const std::string &modesArgsStr)
{
    std::queue<std::string> queue;

    std::istringstream iss(modesArgsStr);
    std::string arg;

    while (std::getline(iss, arg, ' '))
        queue.push(arg);
    return queue;
}

std::pair<queue_str, queue_str> ModeCommand::parseArgs(const std::string &args)
{
    std::pair<std::string, std::string> p = split(args, ' ');
    std::queue<std::string> modes = parseModes(p.first);
    std::queue<std::string> modesArgs = parseModesArgs(p.second);
    return std::make_pair(modes, modesArgs);
}

void ModeCommand::validateModesArgs(User &user, const std::string &modes, queue_str modesArgs, const std::string &channelTag)
{
    char sign = '+';
    size_t numberOfArgsNeeded = 0;
    for (size_t i = 0; i < modes.length(); i++)
    {
        char mode = modes[i];
        switch (mode)
        {
        case 'k':
        case 'b':
        case 'o':
        case 'v':
            numberOfArgsNeeded++;
            break;
        case 'l':
            if (sign == '+')
                numberOfArgsNeeded++;
            break;
        case '-':
            sign = '-';
            break;
        case '+':
            sign = '+';
            break;
        }
    }

    if (numberOfArgsNeeded > modesArgs.size())
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));

    // validate nickname
    for (size_t i = 0; i < modes.length(); i++)
    {
        char mode = modes[i];
        if (mode != 'o' && mode != 'v')
            continue;

        std::string targetNickname = modesArgs.front();
        modesArgs.pop();
        User *targetUser = context->findRegistredUserByNickname(targetNickname);

        if (!targetUser)
            throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", user.nickname, targetNickname));
        if (!targetUser->isJoinedChannel(channelTag))
            throw std::invalid_argument(Error::ERR_USERNOTINCHANNEL("localhost", user.nickname, targetUser->nickname, channelTag));
    }
}

void ModeCommand::validate(User &user, const std::string &args)
{
    std::cout << "mode command validation |" << args << "|\n";

    std::pair<std::string, std::string> p = split(args, ' ');
    std::string channelTag = p.first;
    std::string modesStr = getFirstWord(p.second.c_str());
    std::pair<queue_str, queue_str> pmodes = parseArgs(p.second);
    queue_str modes = pmodes.first;
    queue_str modesArgs = pmodes.second;

    if (channelTag.empty())
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));
    if (!context->isChannelExist(channelTag))
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, channelTag));
    if (modes.empty())
        return;
    this->validateModesArgs(user, modesStr, modesArgs, channelTag);
    user.canManageChannelModes(channelTag);
}

void ModeCommand::run(User &user, const std::string &args)
{
    // mode [channelTag] [modes] [modesArgs]

    std::pair<std::string, std::string> p = split(args, ' ');

    Channel *ch = context->getChannel(p.first);

    std::pair<queue_str, queue_str> pmodes = parseArgs(p.second);
    queue_str modes = pmodes.first;
    queue_str modesArgs = pmodes.second;

    if (modes.empty())
    {
        std::ostringstream oss;
        oss << ":localhost 324 " << user.nickname << " " << ch->getTag() << " " << ch->getModes(user) << std::endl;
        user.send(oss.str());
    }

    while (!modes.empty())
    {
        std::string item = modes.front();
        modes.pop();
        char sign = item[0];
        char mode = item[1];
        switch (mode)
        {
        case 'i':
            ch->toggleMode(user, sign, 'i');
            break;
        case 'l':
            if (sign == '-')
            {
                ch->toggleLimit(user, sign, "");
            }
            else
            {
                ch->toggleLimit(user, sign, modesArgs.front());
                modesArgs.pop();
            }
            break;
        case 'k':
            ch->toggleKey(user, sign, modesArgs.front());
            modesArgs.pop();
            break;
        case 'm':
            ch->toggleMode(user, sign, 'm');
            break;
        case 'n':
            ch->toggleMode(user, sign, 'n');
            break;
        case 't':
            ch->toggleMode(user, sign, 't');
            break;
        case 'b':
            ch->toggleUserBanStatus(user, sign, modesArgs.front());
            modesArgs.pop();
            break;
        case 'o':
            ch->toggleUserOpStatus(user, sign, modesArgs.front());
            modesArgs.pop();
            break;
        case 'v':
            ch->toggleUserVoicedStatus(user, sign, modesArgs.front());
            modesArgs.pop();
            break;
        }
    }

    std::cout << "running mode command\n";
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
    user.canSendPrivMessage(p.first);
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

/**
 * INVITE COMMAND
 */
InviteCommand::InviteCommand(Context *context)
    : CmdHandler(context)
{
}

void InviteCommand::validate(User &user, const std::string &args)
{
    std::queue<std::string> q = splitChunks(args, ' ');
    if (q.size() < 2)
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));
    std::string targetNickname = q.front();
    q.pop();
    std::string channelTag = q.front();
    q.pop();

    if (!context->isNickNameRegistred(targetNickname))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", user.nickname, targetNickname));
    if (!context->isChannelExist(channelTag))
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL("localhost", user.nickname, channelTag));
    user.canInviteUsers(channelTag);
}

void InviteCommand::run(User &user, const std::string &args)
{
    std::queue<std::string> q = splitChunks(args, ' ');
    std::string targetNickname = q.front();
    q.pop();
    std::string channelTag = q.front();
    q.pop();

    Channel *ch = context->getChannel(channelTag);
    RegistredUser *targetUser = context->findRegistredUserByNickname(targetNickname);
    if (!ch || !targetUser)
        return;

    ch->inviteUser(*targetUser);

    {
        std::ostringstream oss;
        oss << ":localhost " << 341 << " " << user.nickname << " " << targetNickname << " " << ch->getTag() << std::endl;
        user.send(oss.str());
    }

    std::ostringstream oss;
    oss << user.getMsgPrefix() << " INVITE " << targetNickname << " :" << channelTag << std::endl;
    targetUser->send(oss.str());
}

/**
 * TOPIC COMMAND
 */
TopicCommand::TopicCommand(Context *context)
    : CmdHandler(context)
{
}

void TopicCommand::validate(User &user, const std::string &args)
{
    std::pair<std::string, std::string> p = split(args, ' ');
    std::string channelTag = p.first;
    if (channelTag.empty())
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS("localhost", user.nickname));
    if (!context->isChannelExist(channelTag))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK("localhost", user.nickname, p.first));
    user.canManageChannelTopic(channelTag, !p.second.empty());
}

void TopicCommand::run(User &user, const std::string &args)
{
    std::ostringstream oss;
    std::pair<std::string, std::string> p = split(args, ' ');
    std::string channelTag = p.first;
    Channel *ch = context->getChannel(channelTag);
    if (!ch)
        return;
    if (p.second.empty())
    {
        if (ch->hasTopic())
            oss << ":localhost 332 " << user.nickname << " " << ch->getTag() << " :" << ch->getTopic() << std::endl;
        else
            oss << ":localhost 331 " << user.nickname << " " << ch->getTag() << " :No topic is set." << std::endl;
        user.send(oss.str());
    }
    else if (p.second == ":")
    {
        ch->setTopic("");
        oss << user.getMsgPrefix() << " TOPIC " << ch->getTag() << " :" << ch->getTopic() << std::endl;
    }
    else
    {
        std::string newTopic = p.second[0] == ':' ? p.second.substr(1) : getFirstWord(p.second.c_str());
        ch->setTopic(newTopic);
        oss << user.getMsgPrefix() << " TOPIC " << ch->getTag() << " :" << ch->getTopic() << std::endl;
        ch->broadcast(oss.str());
    }
}
