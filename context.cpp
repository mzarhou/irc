#include "context.hpp"
#include <unistd.h>

Context::Context(const std::string passw) : serverpassw(passw)
{
    // TODO: add new commands here
    this->registerCommand("USER", new UserCommand(this));
    this->registerCommand("NICK", new NickCommand(this));
    this->registerCommand("PASS", new PassCommand(this));
    this->registerCommand("LIST", new ListCommand(this));
    this->registerCommand("JOIN", new JoinCommand(this));
}

Context::~Context()
{
    COMMANDS_MAP::iterator it = commands.begin();
    for (; it != commands.end(); it++)
    {
        delete it->second;
    }
}

void Context::registerCommand(const std::string &name, CmdHandler *handler)
{
    if (getCommand(name))
        throw std::invalid_argument("command already exists");
    commands[name] = handler;
}

void Context::addNewUser(int sockfd)
{
    GuestUser new_user(this, sockfd);
    guest_users[sockfd] = new_user;
}

CmdHandler *Context::getCommand(const std::string &name)
{
    COMMANDS_MAP::const_iterator it = this->commands.find(name);

    if (it == commands.end())
        return nullptr;
    return it->second;
}

User *Context::getSocketHandler(int sockfd)
{
    GUEST_USERS_MAP::iterator guest_pos = guest_users.find(sockfd);
    if (guest_pos != guest_users.end())
    {
        return &guest_pos->second;
    }
    REGISTRED_USERS_MAP::iterator registred_pos = registred_users.find(sockfd);
    if (registred_pos != registred_users.end())
    {
        return &registred_pos->second;
    }
    throw std::invalid_argument("invalid socket file descriptor");
}

REGISTRED_USERS_MAP::iterator Context::findRegistredUserByNickname(const std::string &nickname)
{
    REGISTRED_USERS_MAP::iterator it = registred_users.begin();
    for (; it != registred_users.end(); it++)
    {
        if (it->second.nickname == nickname)
        {
            return it;
        }
    }
    return it;
}

GUEST_USERS_MAP::iterator Context::findGuestUserByNickName(const std::string &nickname)
{
    GUEST_USERS_MAP::iterator it = guest_users.begin();
    for (; it != guest_users.end(); it++)
    {
        if (it->second.nickname == nickname)
            return it;
    }
    return it;
}

bool Context::isNickNameRegistred(const std::string &nickname)
{
    REGISTRED_USERS_MAP::const_iterator it = findRegistredUserByNickname(nickname);
    return (it != registred_users.end());
}
bool Context::isNickNameGuest(const std::string &nickname)
{
    GUEST_USERS_MAP::iterator it = findGuestUserByNickName(nickname);
    return (it != guest_users.end());
}

bool Context::isUserRegistred(const User &user)
{
    REGISTRED_USERS_MAP::const_iterator it = registred_users.find(user.fd);
    return (it != registred_users.end());
}

bool Context::isUserGuest(const User &user)
{
    GUEST_USERS_MAP::const_iterator it = guest_users.find(user.fd);
    return (it != guest_users.end());
}

void Context::disconnectUser(int fd)
{
    guest_users.erase(fd);

    REGISTRED_USERS_MAP::iterator registred_pos = registred_users.find(fd);
    if (registred_pos != registred_users.end())
    {
        registred_users.erase(registred_pos);
    }
    close(fd);
}

void Context::disconnectUser(const std::string &nickname)
{
    GUEST_USERS_MAP::iterator guest_user_it = findGuestUserByNickName(nickname);
    if (guest_user_it != guest_users.end())
    {
        disconnectUser(guest_user_it->second.fd);
    }
    else
    {
        REGISTRED_USERS_MAP::const_iterator registred_user_it = findRegistredUserByNickname(nickname);
        if (registred_user_it != registred_users.end())
        {
            disconnectUser(registred_user_it->second.fd);
        }
    }
}

void Context::registerUser(GuestUser &user)
{
    std::cout << "registring new user " << user.nickname << std::endl;
    guest_users.erase(user.fd);

    RegistredUser ruser(this, user.fd);
    ruser.setNickname(user.nickname);
    ruser.setRealname(user.realname);
    ruser.setUsername(user.username);
    ruser.setPassword(user.password);

    registred_users[user.fd] = ruser;
}

std::string Context::getServerpassw(void)
{
    return (this->serverpassw);
}

Channel &Context::createNewChannel(const std::string &tag)
{
    Channel c(this, tag);

    channels[tag] = c;
    CHANNELS_MAP::iterator it = channels.find(tag);
    if (it == channels.end())
    {
        std::ostringstream oss;
        oss << "UNKOWN error occured when creating channel " << tag << std::endl;
        throw std::invalid_argument(oss.str());
    }
    return it->second;
}

void Context::joinUserToChannel(User &user, const std::string &tag)
{
    REGISTRED_USERS_MAP::iterator it = registred_users.find(user.fd);
    if (it == registred_users.end())
        throw std::invalid_argument("User is not registred");
    Channel &ch = createNewChannel(tag);
    ch.addNewUser(it->second);
}
