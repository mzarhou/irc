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
    std::string args = getFirstWord(_args.c_str());
    if (args != "time" && args != "hello" && args != "joke" && args != "help" && args != "help -a")
        throw std::invalid_argument(Error::ERR_NOTVALIDPARAM(Server::getHostname(), user.nickname));
}

size_t WriteCallback(char *contents, size_t size, size_t nmemb, std::string *output)
{
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
        CURL *curl;
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
        data = data.erase(0, 1);
        data = data.erase(data.length() - 1, data.length());
        std::istringstream ss(data);
        std::string token;
        int i = 0;
        while (std::getline(ss, token, ':'))
            i++;
        token = token.erase(0, 1);
        token = token.erase(token.length() - 1, token.length());
        token += "\n";
        user.send(token);
        std::cout << "SEND JOKE\n";
    }
    if (args == "help" || args == "help -a")
    {
        std::cout << "Available commands:\n";

        // Create a map to store command information
        std::map<std::string, botCommand> commandMap;

        // Register commands and their details
        commandMap["USER"] = (botCommand){"USER", "The USER command is used at the beginning of a connection to specify the username and realname of a new user.\nIt must be noted that <realname> must be the last parameter because it may contain SPACE (' ', 0x20) characters, and should be prefixed with a colon (:) if required.\nServers MAY use the Ident Protocol to look up the ‘real username’ of clients. If username lookups are enabled and a client does not have an Identity Server enabled, the username provided by the client SHOULD be prefixed by a tilde ('~', 0x7E) to show that this value is user-set.", "Command: USER\nParameters: <username> 0 * <realname>"};
        commandMap["NICK"] = (botCommand){"NICK", "The NICK command is used to give the client a nickname or change the previous one.\nIf the server receives a NICK command from a client where the desired nickname is already in use on the network, it should issue an ERR_NICKNAMEINUSE numeric and ignore the NICK command.\nIf the server does not accept the new nickname supplied by the client as valid (for instance, due to containing invalid characters), it should issue an ERR_ERRONEUSNICKNAME numeric and ignore the NICK command.\nIf the server does not receive the <nickname> parameter with the NICK command, it should issue an ERR_NONICKNAMEGIVEN numeric and ignore the NICK command.", "Command: NICK\nParameters: <nickname>"};
        commandMap["PASS"] = (botCommand){"PASS", "The PASS command is used to set a ‘connection password’. If set, the password must be set before any attempt to register the connection is made. This requires that clients send a PASS command before sending the NICK / USER combination.\nThe password supplied must match the one defined in the server configuration. It is possible to send multiple PASS commands before registering but only the last one sent is used for verification and it may not be changed once the client has been registered.", "Command: PASS\nParameters: <password>"};
        commandMap["JOIN"] = (botCommand){"JOIN", "The JOIN command indicates that the client wants to join the given channel(s), each channel using the given key for it. The server receiving the command checks whether or not the client can join the given channel, and processes the request. Servers MUST process the parameters of this command as lists on incoming commands from clients, with the first <key> being used for the first <channel>, the second <key> being used for the second <channel>, etc.", " Command: JOIN \nParameters: <channel>{,<channel>} [<key>{,<key>}]"};
        commandMap["PART"] = (botCommand){"PART", "The PART command removes the client from the given channel(s). On sending a successful PART command, the user will receive a PART message from the server for each channel they have been removed from. <reason> is the reason that the client has left the channel(s).\nFor each channel in the parameter of this command, if the channel exists and the client is not joined to it, they will receive an ERR_NOTONCHANNEL (442) reply and that channel will be ignored. If the channel does not exist, the client will receive an ERR_NOSUCHCHANNEL (403) reply and that channel will be ignored.\nThis message may be sent from a server to a client to notify the client that someone has been removed from a channel. In this case, the message <source> will be the client who is being removed, and <channel> will be the channel which that client has been removed from. Servers SHOULD NOT send multiple channels in this message to clients, and SHOULD distribute these multiple-channel PART messages as a series of messages with a single channel name on each. If a PART message is distributed in this way, <reason> (if it exists) should be on each of these messages.", "Command: PART\nParameters: <channel>{,<channel>} [<reason>]"};
        commandMap["PRIVMSG"] = (botCommand){"PRIVMSG", "The PRIVMSG command is used to send private messages between users, as well as to send messages to channels. <target> is the nickname of a client or the name of a channel.\nIf <target> is a channel name and the client is banned and not covered by a ban exception, the message will not be delivered and the command will silently fail. Channels with the moderated mode active may block messages from certain users. Other channel modes may affect the delivery of the message or cause the message to be modified before delivery, and these modes are defined by the server software and configuration being used.\nIf a message cannot be delivered to a channel, the server SHOULD respond with an ERR_CANNOTSENDTOCHAN (404) numeric to let the user know that this message could not be delivered.\nIf <target> is a channel name, it may be prefixed with one or more channel membership prefix character (@, +, etc) and the message will be delivered only to the members of that channel with the given or higher status in the channel. Servers that support this feature will list the prefixes which this is supported for in the STATUSMSG RPL_ISUPPORT parameter, and this SHOULD NOT be attempted by clients unless the prefix has been advertised in this token.\nIf <target> is a user and that user has been set as away, the server may reply with an RPL_AWAY (301) numeric and the command will continue.\nThe PRIVMSG message is sent from the server to client to deliver a message to that client. The <source> of the message represents the user or server that sent the message, and the <target> represents the target of that PRIVMSG (which may be the client, a channel, etc).\nWhen the PRIVMSG message is sent from a server to a client and <target> starts with a dollar character ('$', 0x24), the message is a broadcast sent to all clients on one or multiple servers.", "Command: PRIVMSG\nParameters: <target>{,<target>} <text to be sent>"};
        commandMap["QUIT"] = (botCommand){"QUIT", "The QUIT command is used to terminate a client’s connection to the server. The server acknowledges this by replying with an ERROR message and closing the connection to the client.\nThis message may also be sent from the server to a client to show that a client has exited from the network. This is typically only dispatched to clients that share a channel with the exiting user. When the QUIT message is sent to clients, <source> represents the client that has exited the network.\nWhen connections are terminated by a client-sent QUIT command, servers SHOULD prepend <reason> with the ASCII string \"Quit: \" when sending QUIT messages to other clients, to represent that this user terminated the connection themselves. This applies even if <reason> is empty, in which case the reason sent to other clients SHOULD be just this \"Quit: \" string. However, clients SHOULD NOT change behaviour based on the prefix of QUIT message reasons, as this is not required behaviour from servers.\nWhen a netsplit (the disconnecting of two servers) occurs, a QUIT message is generated for each client that has exited the network, distributed in the same way as ordinary QUIT messages. The <reason> on these QUIT messages SHOULD be composed of the names of the two servers involved, separated by a SPACE (' ', 0x20). The first name is that of the server which is still connected and the second name is that of the server which has become disconnected. If servers wish to hide or obscure the names of the servers involved, the <reason> on these messages MAY also be the literal ASCII string \"*.net *.split\" (i.e. the two server names are replaced with \"*.net\" and \"*.split\"). Software that implements the IRCv3 batch Extension should also look at the netsplit and netjoin batch types.\nIf a client connection is closed without the client issuing a QUIT command to the server, the server MUST distribute a QUIT message to other clients informing them of this, distributed in the same was an ordinary QUIT message. Servers MUST fill <reason> with a message reflecting the nature of the event which caused it to happen. For instance, \"Ping timeout: 120 seconds\", \"Excess Flood\", and \"Too many connections from this IP\" are examples of relevant reasons for closing or for a connection with a client to have been closed.", "Command: QUIT\nParameters: [<reason>]"};
        commandMap["INVITE"] = (botCommand){"INVITE", "The INVITE command is used to invite a user to a channel. The parameter <nickname> is the nickname of the person to be invited to the target channel <channel>.\nThe target channel SHOULD exist (at least one user is on it). Otherwise, the server SHOULD reject the command with the ERR_NOSUCHCHANNEL numeric.\nOnly members of the channel are allowed to invite other users. Otherwise, the server MUST reject the command with the ERR_NOTONCHANNEL numeric.\nServers MAY reject the command with the ERR_CHANOPRIVSNEEDED numeric. In particular, they SHOULD reject it when the channel has invite-only mode set, and the user is not a channel operator.\nIf the user is already on the target channel, the server MUST reject the command with the ERR_USERONCHANNEL numeric.\nWhen the invite is successful, the server MUST send a RPL_INVITING numeric to the command issuer, and an INVITE message, with the issuer as <source>, to the target user. Other channel members SHOULD NOT be notifie", "Command: INVITE\nParameters: <nickname> <channel>"};
        commandMap["TOPIC"] = (botCommand){"TOPIC", "The TOPIC command is used to change or view the topic of the given channel. If <topic> is not given, either RPL_TOPIC or RPL_NOTOPIC is returned specifying the current channel topic or lack of one. If <topic> is an empty string, the topic for the channel will be cleared.\nIf the client sending this command is not joined to the given channel, and tries to view its’ topic, the server MAY return the ERR_NOTONCHANNEL (442) numeric and have the command fail.\nIf RPL_TOPIC is returned to the client sending this command, RPL_TOPICWHOTIME SHOULD also be sent to that client.\nIf the protected topic mode is set on a channel, then clients MUST have appropriate channel permissions to modify the topic of that channel. If a client does not have appropriate channel permissions and tries to change the topic, the ERR_CHANOPRIVSNEEDED (482) numeric is returned and the command will fail.\nIf the topic of a channel is changed or cleared, every client in that channel (including the author of the topic change) will receive a TOPIC command with the new topic as argument (or an empty argument if the topic was cleared) alerting them to how the topic has changed.\nClients joining the channel in the future will receive a RPL_TOPIC numeric (or lack thereof) accordingly.", "Command: TOPIC\nParameters: <channel> [<topic>]"};
        commandMap["KICK"] = (botCommand){"KICK", "The KICK command can be used to request the forced removal of a user from a channel. It causes the <user> to be removed from the <channel> by force. If no comment is given, the server SHOULD use a default message instead.\nThe server MUST NOT send KICK messages with multiple users to clients. This is necessary to maintain backward compatibility with existing client software.\nServers MAY limit the number of target users per KICK command via the TARGMAX parameter of RPL_ISUPPORT, and silently drop targets if the number of targets exceeds the limit.", "Command: KICK\nParameters: <channel> <user> *( \",\" <user> ) [<comment>]"};

        // Add more commands and their details here...

        // Iterate over the command map and print the details
        std::map<std::string, botCommand>::const_iterator iter = commandMap.begin();
        for (; iter != commandMap.end(); ++iter)
        {
            // std::cout << iter->first << " - " << iter->first << "\n";
            // std::cout << "Usage: " << iter->second.usage << "\n\n";
            std::ostringstream oss;
            oss
                << iter->first << " - " << iter->first << "\n";
            if (args == "help -a")
                oss << "description: " << iter->second.description << "\n";

            oss << "Usage: " << iter->second.usage << "\n\n";
            user.send(oss.str());
        }
    }
}
