#include "channel.hpp"

Channel::Channel(Context *context, const std::string &tag) : context(context), tag(tag)
{
}

Channel::Channel() : context(NULL), tag("")
{
}

Channel::~Channel()
{
    (void)context;
}

std::string Channel::getTag()
{
    return this->tag;
}

Channel &Channel::operator=(const Channel &other)
{
    this->context = other.context;
    this->tag = other.tag;
    this->users = other.users;
    return *this;
}

bool Channel::isUserOp(const User &user) const
{
    REGISTRED_USERS_MAP::const_iterator opsIt = operators.find(user.fd);
    return (opsIt != operators.end());
}

std::string Channel::getUsersStr()
{
    std::string usersStr = "";
    REGISTRED_USERS_MAP::iterator usersIt = users.begin();
    for (; usersIt != users.end(); usersIt++)
    {
        if (!this->isUserOp(usersIt->second))
        {
            usersStr += usersIt->second.nickname;
            usersStr += " ";
        }
    }
    REGISTRED_USERS_MAP::iterator opsIt = operators.begin();
    for (; opsIt != operators.end(); opsIt++)
    {
        usersStr += "@";
        usersStr += opsIt->second.nickname;
        usersStr += " ";
    }
    return usersStr;
}

void Channel::addNewUser(RegistredUser &user)
{
    if (this->empty())
        operators[user.fd] = user;
    users[user.fd] = user;
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

bool Channel::hasUser(const User &user)
{
    REGISTRED_USERS_MAP::iterator it = users.find(user.fd);
    return (it != users.end());
}

void Channel::kickUser(const User &user)
{
    users.erase(user.fd);
    operators.erase(user.fd);
    if (this->empty())
        context->deleteChannel(*this);
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

bool Channel::isUserOp(const User &user) const
{
    REGISTRED_USERS_MAP::const_iterator it = operators.find(user.fd);
    return (it != operators.end());
}

// channel specific modes
void Channel::toggleInviteOnlyStatus(char sign)
{
}

void Channel::toggleModeratedStatus(char sign)
{
}

void Channel::toggleNoExternalMsgStatus(char sign)
{
}

void Channel::toggleOpsOnlyCanChangeTopicStatus(char sign)
{
}

void Channel::setLimit(const std::string &limit)
{
    (void)limit;
    // TODO: parse limit, if it's invalid ignore it
}

// user specific modes
void Channel::toggleUserBanStatus(char sign, const std::string &targetNickname)
{
}

void Channel::toggleUserVoicedStatus(char sign, const std::string &targetNickname)
{
}

void Channel::toggleUserOpStatus(char sign, const std::string &targetNickname)
{
}

/**
 * send message all users in the channel
 */
void Channel::broadcast(const std::string &message)
{
    REGISTRED_USERS_MAP::iterator it = users.begin();
    for (; it != users.end(); it++)
        it->second.send(message);
}

/**
 * send message all users in the channel
 * excluding current authenticated user
 */
void Channel::emit(const User &userToExclude, const std::string &message)
{
    REGISTRED_USERS_MAP::iterator it = users.begin();
    for (; it != users.end(); it++)
    {
        if (it->second.fd == userToExclude.fd)
            continue;
        it->second.send(message);
    }
}

bool Channel::empty()
{
    return (users.begin() == users.end());
}
