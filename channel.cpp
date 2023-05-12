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
    users[user.fd] = user;
}

bool Channel::hasUser(User &user)
{
    REGISTRED_USERS_MAP::iterator it = users.find(user.fd);
    return (it != users.end());
}

void Channel::kickUser(User &user)
{
    users.erase(user.fd);
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
