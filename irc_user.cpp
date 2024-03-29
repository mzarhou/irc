#include "irc_user.hpp"

/**
 * User
 */
User::User(Context *context, int sockfd, const std::string &ip)
    : context(context), fd(sockfd), nickname(""), username(""), buffer(""), ip(ip)
{
}

User::User(const User &other)
{
    *this = other;
}

void User::canManageChannelModes(const Channel &ch)
{
    if (!this->isChannelOp(ch))
        throw std::invalid_argument(Error::ERR_CHANOPRIVSNEEDED(Server::getHostname(), this->nickname, ch.getTag()));
}

void User::canManageChannelModes(const std::string &validChannelTag)
{
    Channel *ch = context->getChannel(validChannelTag);
    if (!ch)
        return;
    this->canManageChannelModes(*ch);
}

void User::canManageChannelTopic(const std::string &channelTag, bool isEditingTopic)
{
    Channel *ch = context->getChannel(channelTag);
    if (!ch)
        return;
    if (isEditingTopic && !this->isChannelOp(*ch))
        throw std::invalid_argument(Error::ERR_CHANOPRIVSNEEDED(Server::getHostname(), this->nickname, ch->getTag()));
}

void User::canInviteUsers(const Channel &ch)
{
    if (!this->isChannelOp(ch))
        throw std::invalid_argument(Error::ERR_CHANOPRIVSNEEDED(Server::getHostname(), this->nickname, ch.getTag()));
}

void User::canInviteUsers(const std::string &validChannelTag)
{
    Channel *ch = context->getChannel(validChannelTag);
    if (!ch)
        return;
    this->canInviteUsers(*ch);
}

void User::canSendPrivMessage(const std::string &validChannelTagOrNickname)
{
    if (validChannelTagOrNickname.empty() || validChannelTagOrNickname[0] != '#')
        return;

    Channel *ch = context->getChannel(validChannelTagOrNickname);
    if (!ch || ch->isUserOp(*this) || ch->isUserVoiced(*this))
        return;
    if (ch->moderated() || (!ch->hasUser(*this) && !ch->externalMsgsAllowed()))
    {
        std::ostringstream oss;
        oss << ":" << Server::getHostname() << " " << 404 << " " << this->nickname << " " << ch->getTag() << " :Cannot send to nick/channel\n";
        throw std::invalid_argument(oss.str());
    }
}

void User::canJoinChannel(const User &user, const Channel &ch, const std::string &key)
{
    std::ostringstream oss;

    oss << ":" << Server::getHostname() << " ";
    if (ch.isInviteOnly() && !ch.isUserInvited(user))
    {
        oss << 473 << " " << this->nickname << " " << ch.getTag() << " :Cannot join channel (+i) - you must be invited\n";
        throw std::invalid_argument(oss.str());
    }

    if (ch.isUserBanned(*this))
    {
        oss << 474 << " " << this->nickname << " " << ch.getTag() << " :Cannot join channel (+b) - you are banned\n";
        throw std::invalid_argument(oss.str());
    }

    if (ch.requireAuth() && key != ch.getKey())
    {
        oss << 475 << " " << this->nickname << " " << ch.getTag() << " :Cannot join channel (+k) - bad key\n";
        throw std::invalid_argument(oss.str());
    }

    if (!ch.checkLimit())
    {
        oss << 471 << " " << this->nickname << " " << ch.getTag() << " :Cannot join channel (+l) - channel is full, try again later\n";
        throw std::invalid_argument(oss.str());
    }
}

void User::canJoinChannel(const User &user, const std::string &channelTag, const std::string &key)
{
    Channel *ch = context->getChannel(channelTag);
    if (!ch)
        return;
    return this->canJoinChannel(user, *ch, key);
}

bool User::isRegistred()
{
    return context->isUserRegistred(*this);
}

bool User::isGuest()
{
    return context->isUserGuest(*this);
}

bool User::isChannelOp(const Channel &ch)
{
    return ch.isUserOp(*this);
}

bool User::isChannelOp(const std::string &channelTag)
{
    Channel *ch = context->getChannel(channelTag);
    if (!ch)
        return false;
    return isChannelOp(*ch);
}

void User::send(const std::string &msg) const
{
    if (msg.length() == 0)
        return;
    if (::send(this->fd, msg.c_str(), msg.length(), 0) == -1)
    {
        perror("send");
    }
}

/**
 * send message to all users in all user channels
 * send message one time only for each user
 */
void User::sendToUserChannels(const std::string &message)
{
    std::vector<Channel *> channels = this->channels();
    std::vector<Channel *>::iterator it = channels.begin();
    std::map<int, int> usersFds;

    for (; it != channels.end(); it++)
    {
        REGISTRED_USERS_MAP users = (*it)->getUsers();
        REGISTRED_USERS_MAP::iterator usersIt = users.begin();
        for (; usersIt != users.end(); usersIt++)
        {
            if (usersFds.find(usersIt->second.fd) != usersFds.end())
                continue;
            usersIt->second.send(message);
            usersFds[usersIt->second.fd] = usersIt->second.fd;
        }
    }
}

bool User::isJoinedChannel(const std::string &channelTag)
{
    Channel *ch = context->getChannel(channelTag);
    if (!ch)
        return false;
    return ch->hasUser(*this);
}

std::vector<Channel *> User::channels()
{
    return context->getUserChannels(*this);
}

std::string User::getMsgPrefix() const
{
    std::ostringstream oss;
    oss << ":" << nickname << "!" << username << "@" << this->ip;
    return oss.str();
}

User::~User() {}

void User::setNickname(const std::string &value)
{
    this->nickname = value;
}

void User::setRealname(const std::string &value)
{
    this->realname = value;
}

void User::setUsername(const std::string &value)
{
    this->username = value;
}

void User::setPassword(const std::string &value)
{
    this->password = value;
}

/**
 * GuestUser
 */
GuestUser::GuestUser() : User(NULL, -1, "") {}
GuestUser::GuestUser(Context *context, int sockfd, const std::string &ip) : User(context, sockfd, ip) {}
GuestUser::GuestUser(const GuestUser &other) : User(other)
{
    *this = other;
}
GuestUser::~GuestUser() {}

void GuestUser::handleSocket(const Command &cmd)
{
    CmdHandler *command = context->getCommand(cmd.name);
    if (!command)
    {
        return;
    }

    /**
     * user must register before sending other valid commands
     * than this ones
     */
    std::string cmds[] = {"PASS", "USER", "NICK", "QUIT"};
    std::string *it = std::find(std::begin(cmds), std::end(cmds), cmd.name);
    if (it == std::end(cmds))
    {
        std::ostringstream oss;
        oss << ":" << Server::getHostname() << " 451 * LIST :You must finish connecting with another nickname first.\n";
        this->send(oss.str());
    }
    else
    {
        try
        {
            command->validate(*this, cmd.args);
            command->run(*this, cmd.args);
            onChange();
        }
        catch (const std::exception &e)
        {
            this->send(e.what());
        }
    }
}

void GuestUser::onChange()
{
    if (nickname.empty() || username.empty() || password.empty())
        return;

    std::ostringstream oss;
    oss << ":" << Server::getHostname() << " 001 " << nickname << " :Welcome to the 1337.server.chat Internet Relay Chat Network " << nickname << "\n";
    oss << ":" << Server::getHostname() << " 002 " << nickname << " :Your host is irc, running version 1.1.2\r\n";
    oss << ":" << Server::getHostname() << " 003 " << nickname << " :This server was created \n";
    oss << ":" << Server::getHostname() << " 004 " << nickname << " :" << Server::getHostname() << " 1.0 - -\r\n";
    this->send(oss.str());
    context->registerUser(*this);
}

/**
 * RegistredUser
 */
RegistredUser::RegistredUser() : User(NULL, -1, "") {}
RegistredUser::RegistredUser(Context *context, int sockfd, const std::string &ip) : User(context, sockfd, ip) {}
RegistredUser::RegistredUser(const RegistredUser &other) : User(other)
{
    *this = other;
}
RegistredUser::~RegistredUser()
{
}

void RegistredUser::handleSocket(const Command &cmd)
{
    CmdHandler *command = context->getCommand(cmd.name);
    if (!command)
    {
        std::ostringstream oss;
        oss << ": " << Server::getHostname() << " 421 " << nickname << " " << cmd.originalName << " :Unknown command\n";
        this->send(oss.str());
        return;
    }

    try
    {
        command->validate(*this, cmd.args);
        command->run(*this, cmd.args);
    }
    catch (const std::exception &e)
    {
        this->send(e.what());
    }
}
