#include "irc_user.hpp"

/**
 * User
 */
User::User(Context *context, int sockfd) : context(context), fd(sockfd), nickname(""), username(""){};

User::User(const User &other)
{
    *this = other;
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
    // TODO: change localhost with user ip
    oss << ":" << nickname << "!" << username << "@localhost";
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
GuestUser::GuestUser() : User(NULL, -1) {}
GuestUser::GuestUser(Context *context, int sockfd) : User(context, sockfd) {}
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
        this->send(":localhost 451 * LIST :You must finish connecting with another nickname first.\n");
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
    std::cout << "-> socket fd: " << fd << '\n';
    std::cout << "-> nickname: " << nickname << '\n';
    std::cout << "-> username: " << username << '\n';
    std::cout << "-> password: " << password << '\n';
    if (nickname.empty() || username.empty() || password.empty())
        return;

    std::ostringstream oss;
    oss << ":localhost " << nickname << " :Welcome to the 1337.server.chat Internet Relay Chat Network " << nickname << '\n';
    this->send(oss.str());
    context->registerUser(*this);
}

/**
 * RegistredUser
 */
RegistredUser::RegistredUser() : User(NULL, -1) {}
RegistredUser::RegistredUser(Context *context, int sockfd) : User(context, sockfd) {}
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
        oss << ":localhost 421 " << nickname << " " << cmd.originalName << " :Unknown command\n";
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
