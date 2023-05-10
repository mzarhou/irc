#include "context.hpp"

Context::Context(const std::string passw) : serverpassw(passw)
{
    // TODO: add new commands here
    this->registerCommand("USER", new UserCommand(this));
    this->registerCommand("NICK", new NickCommand(this));
    this->registerCommand("PASS", new PassCommand(this));
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
    this->last_connected = sockfd;
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
    REGISTRED_USERS_MAP::iterator registred_pos = findRegistredUserByFd(sockfd);
    if (registred_pos != registred_users.end())
    {
        return &registred_pos->second;
    }
    throw std::invalid_argument("invalid socket file descriptor");
}

REGISTRED_USERS_MAP::iterator Context::findRegistredUserByFd(int fd)
{
    REGISTRED_USERS_MAP::iterator it = registred_users.begin();
    while (it != registred_users.end())
    {
        if (it->second.fd == fd)
        {
            return it;
        }
    }
    return it;
}

void Context::onUserDeconnected(int fd)
{
    connected_users.erase(fd);

    REGISTRED_USERS_MAP::iterator registred_pos = findRegistredUserByFd(fd);
    if (registred_pos != registred_users.end())
    {
        registred_users.erase(registred_pos);
    }
}

void Context::registerUser(ConnectedUser &user)
{
    // TODO: register user
    std::cout << "registring new user " << user.nickname << std::endl;
}

std::string Context::getServerpassw(void)
{
    return (this->serverpassw);
}

void Context::sendClientMsg(int socketfd, std::string msg)
{
    if (send(socketfd, msg.c_str(), msg.length(), 0) == -1) {
        perror("send");
    }
}
