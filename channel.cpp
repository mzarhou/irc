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
