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
