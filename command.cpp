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
        throw std::invalid_argument(":" + Server::getHostname() + " 462 * :You are already registred and cannot handshake again\n");
    else if (args.empty())
        throw std::invalid_argument(":" + Server::getHostname() + " 461 * PASS :Not enough parameters\n");
    else if (args.compare(Server::getPassword()) != 0)
        throw std::invalid_argument(":" + Server::getHostname() + " 464 * PASS :Password incorrect\n");
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

void UserCommand::validate(User &user, const std::string &args)
{
    if (user.isRegistred())
        throw std::invalid_argument(":" + Server::getHostname() + " 462 * :You are already registred and cannot handshake again\n");
    if (user.password.empty())
        throw std::invalid_argument(":" + Server::getHostname() + " * :No password given\n");
    if (splitChunks(args, ' ').size() < 4)
        throw std::invalid_argument(":" + Server::getHostname() + " 461 * USER :Not enough parameters\n");
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
    if (user.password.empty())
        throw std::invalid_argument(":" + Server::getHostname() + " * :No password given\n");
    if (args.empty())
        throw std::invalid_argument(":" + Server::getHostname() + " 431 * :No nickname given\n");
    if (user.nickname == args)
        throw std::invalid_argument("");
    if (context->isNickNameRegistred(args))
    {
        std::ostringstream oss;
        oss << ":" << Server::getHostname() << " 433 * " << args << " :Nickname is already in use.\n";
        throw std::invalid_argument(oss.str());
    }

    /**
     * disconnect old guest user with similar nickname if exists
     */
    if (context->isNickNameGuest(args))
    {
        User *oldGuest = context->findGuestUserByNickName(args);

        std::ostringstream oss;
        oss << "ERROR :Closing Link: " << oldGuest->ip << " (Overridden)\n";
        oldGuest->send(oss.str());
        context->disconnectUser(args);
    }
}

void NickCommand::run(User &user, const std::string &newNickname)
{
    std::cout << "run NickCommand " << std::endl;

    std::ostringstream oss;
    oss << user.getMsgPrefix() << " NICK :" << newNickname << '\n';

    user.nickname = newNickname;

    if (context->isNickNameRegistred(newNickname))
    {
        user.sendToUserChannels(oss.str());
    }
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
                user.send(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, tag));
            else
            {
                std::string message = "JOIN " + tag + " " + key;
                user.handleSocket(Command::fromMessage(message));
            }
        }
        throw std::invalid_argument("");
    }
    else if (tag.length() == 0)
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "JOIN"));
    else if (tag[0] != '#')
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, tag));
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
            oss << ":" << Server::getHostname() << " MODE " << ch->getTag() << " " << ch->getModes(user) << std::endl;
        oss << ":" << Server::getHostname() << " 353 " << user.nickname << " = " << tag << " :" << ch->getUsersStr() << std::endl
            << ":" << Server::getHostname() << " 366 " << user.nickname << " :End of /NAMES list." << std::endl;
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
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "PART"));
    else if (!context->isChannelExist(args))
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, args));
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
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "MODE"));

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
            throw std::invalid_argument(Error::ERR_NOSUCHNICK(Server::getHostname(), user.nickname, targetNickname));
        if (!targetUser->isJoinedChannel(channelTag))
            throw std::invalid_argument(Error::ERR_USERNOTINCHANNEL(Server::getHostname(), user.nickname, targetUser->nickname, channelTag));
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
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "MODE"));
    if (!context->isChannelExist(channelTag))
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, channelTag));
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
        oss << ":" << Server::getHostname() << " 324 " << user.nickname << " " << ch->getTag() << " " << ch->getModes(user) << std::endl;
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
        throw std::invalid_argument(Error::ERR_NORECIPIENT(Server::getHostname(), user.nickname));
    if (p.second.empty())
        throw std::invalid_argument(Error::ERR_NOTEXTTOSEND(Server::getHostname(), user.nickname));
    if (p.first[0] == '#' && !context->isChannelExist(p.first))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK(Server::getHostname(), user.nickname, p.first));
    if (p.first[0] != '#' && !context->isNickNameRegistred(p.first))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK(Server::getHostname(), user.nickname, p.first));
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

    std::ostringstream oss;
    oss << "ERROR :Closing Link: " << user.ip << " (Client Quit)\n";
    user.send(oss.str());
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
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "INVITE"));
    std::string targetNickname = q.front();
    q.pop();
    std::string channelTag = q.front();
    q.pop();

    if (!context->isNickNameRegistred(targetNickname))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK(Server::getHostname(), user.nickname, targetNickname));
    if (!context->isChannelExist(channelTag))
        throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, channelTag));
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
        oss << ":" << Server::getHostname() << " " << 341 << " " << user.nickname << " " << targetNickname << " " << ch->getTag() << std::endl;
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
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "TOPIC"));
    if (!context->isChannelExist(channelTag))
        throw std::invalid_argument(Error::ERR_NOSUCHNICK(Server::getHostname(), user.nickname, p.first));
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
            oss << ":" << Server::getHostname() << " 332 " << user.nickname << " " << ch->getTag() << " :" << ch->getTopic() << std::endl;
        else
            oss << ":" << Server::getHostname() << " 331 " << user.nickname << " " << ch->getTag() << " :No topic is set." << std::endl;
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

/**
 * KICK COMMAND
 */
KickCommand::KickCommand(Context *context)
    : CmdHandler(context)
{
}

int numberOfParam(std::string str)
{
    std::istringstream ss(str);
    std::string token;
    int i = 0;
    while (std::getline(ss, token, ' '))
    {
        i++;
    }
    return (i);
}

void KickCommand::validate(User &user, const std::string &args)
{
    std::istringstream ss(args);
    std::string token, channel;
    Channel *ch;
    RegistredUser *us;
    int i = 0;
    int nb_param = numberOfParam(args);
    if (nb_param < 2)
        throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "KICK"));
    while (std::getline(ss, token, ' '))
    {
        if (i == 0)
        {
            if (token.substr(0, 1) == "#")
            {
                if (!context->isChannelExist(token))
                    throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, token));
                if (!user.isChannelOp(token)) // user.isChannelOp(ch->getTag())
                {
                    std::cout << "debug\n";

                    throw std::invalid_argument(Error::ERR_CHANOPRIVSNEEDED(Server::getHostname(), user.nickname, token));
                }
                channel = token;
            }
            else
                throw std::invalid_argument(Error::ERR_NOSUCHCHANNEL(Server::getHostname(), user.nickname, token));
        }
        if (i == 1)
        {
            if (!context->isNickNameRegistred(token))
                throw std::invalid_argument(Error::ERR_NOSUCHNICK(Server::getHostname(), token, user.nickname));
            ch = context->getChannel(channel);
            us = context->findRegistredUserByNickname(token);
            if (!ch->hasUser(user))
                throw std::invalid_argument(Error::ERR_NOTONCHANNEL(Server::getHostname(), token, channel));
            if (!ch->hasUser(*us))
                throw std::invalid_argument(Error::ERR_USERNOTINCHANNEL(Server::getHostname(), user.nickname, token, channel));
        }
        i++;
    }
    std::cout << "KICK A CLIENT FROM THE CHANNEL" << std::endl;
}

void KickCommand::run(User &user, const std::string &args)
{
    std::istringstream ss(args);
    std::ostringstream oss;
    RegistredUser *us;
    std::string token;
    Channel *ch;
    int i = 0;
    while (std::getline(ss, token, ' '))
    {
        if (i == 0)
        {
            ch = context->getChannel(token);
        }
        if (i == 1)
        {
            us = context->findRegistredUserByNickname(token);
            ch->kickUser(*us);
            oss << user.getMsgPrefix() << " KICK " << ch->getTag() << " " << token << '\n';
            // user.send(oss.str());
            ch->broadcast(oss.str());
        }
        i++;
    }
}

/**
 * BOT COMMAND
 */
BotCommand::BotCommand(Context *context)
    : CmdHandler(context)
{
}

void BotCommand::validate(User &user, const std::string &_args)
{
    // std::istringstream ss(args);
    // std::string token;
    // if (numberOfParam(args) != 1)
    //     throw std::invalid_argument(Error::ERR_NEEDMOREPARAMS(Server::getHostname(), user.nickname, "BOT"));
    std::string args = getFirstWord(_args.c_str());
    if (args != "time" && args != "joke")
        throw std::invalid_argument(Error::ERR_NOTVALIDPARAM(Server::getHostname(), user.nickname));
}

size_t WriteCallback(char* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(contents, totalSize);
    return totalSize;
}

void BotCommand::run(User &user, const std::string &args)
{
    if (args == "time")
    {
        std::ostringstream oss;
        std::time_t currentTime = std::time(0);
        std::string timeString = std::ctime(&currentTime);
        oss << "Current time: " << timeString;
        user.send(oss.str());
    }
    if (args == "joke")
    {
        CURL* curl;
        std::string data;
        curl = curl_easy_init();
        if (curl)
        {
            curl_easy_setopt(curl, CURLOPT_URL, "https://api.chucknorris.io/jokes/random");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK)
            {
                // Handle error
            }
            curl_easy_cleanup(curl);
        }
        data = data.erase(0,1);
        data = data.erase(data.length() - 1, data.length());
        std::istringstream ss(data);
        std::string token;
        int i = 0;
        while (std::getline(ss, token, ':'))
            i++;
        token = token.erase(0,1);
        token = token.erase(token.length() - 1, token.length());
        token += "\n";
        user.send(token);
        std::cout << "SEND JOKE\n";
    }
}
