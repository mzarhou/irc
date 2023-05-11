#include "context.hpp"
#include <unistd.h>

Context::Context(const std::string passw) : serverpassw(passw)
{
    // TODO: add new commands here
    this->registerCommand("USER", new UserCommand(this));
    this->registerCommand("NICK", new NickCommand(this));
    this->registerCommand("PASS", new PassCommand(this));
    this->registerCommand("LIST", new ListCommand(this));
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
    ConnectedUser new_user(this, sockfd);
    connected_users[sockfd] = new_user;
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
    CONNECTED_USERS_MAP::iterator connected_pos = connected_users.find(sockfd);
    if (connected_pos != connected_users.end())
    {
        return &connected_pos->second;
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

CONNECTED_USERS_MAP::iterator Context::findConnectedUsersByNickName(const std::string &nickname)
{
    CONNECTED_USERS_MAP::iterator it = connected_users.begin();
    for (; it != connected_users.end(); it++)
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
bool Context::isNickNameConnected(const std::string &nickname)
{
    CONNECTED_USERS_MAP::iterator it = findConnectedUsersByNickName(nickname);
    return (it != connected_users.end());
}

void Context::disconnectUser(int fd)
{
    connected_users.erase(fd);

    REGISTRED_USERS_MAP::iterator registred_pos = registred_users.find(fd);
    if (registred_pos != registred_users.end())
    {
        registred_users.erase(registred_pos);
    }
    close(fd);
}

void Context::disconnectUser(const std::string &nickname)
{
    CONNECTED_USERS_MAP::iterator connected_user_it = findConnectedUsersByNickName(nickname);
    if (connected_user_it != connected_users.end())
    {
        disconnectUser(connected_user_it->second.fd);
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

void Context::registerUser(ConnectedUser &user)
{
    std::cout << "registring new user " << user.nickname << std::endl;
    connected_users.erase(user.fd);

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

void Context::sendClientMsg(User &user, const std::string &msg)
{
    if (send(user.fd, msg.c_str(), msg.length(), 0) == -1)
    {
        perror("send");
    }
}

void Context::createNewChannel(const std::string &tag)
{
    CHANNELS_MAP::iterator it = channels.find(tag);
    if (it != channels.end())
        throw std::invalid_argument("channel already exists");
    Channel c(this, tag);
    channels[tag] = c;
}

