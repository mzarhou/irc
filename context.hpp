#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <iostream>
#include <map>

class User;
class ConnectedUser;
class RegistredUser;
class CmdHandler;

#include "command.hpp"
#include "irc_user.hpp"

typedef std::map<int, ConnectedUser> CONNECTED_USERS_MAP;
typedef std::map<std::string, RegistredUser> REGISTRED_USERS_MAP;
typedef std::map<std::string, CmdHandler *> COMMANDS_MAP;

class Context
{
private:
    const std::string serverpassw;
    CONNECTED_USERS_MAP connected_users;
    REGISTRED_USERS_MAP registred_users;
    COMMANDS_MAP commands;

    // one context only should exists
    Context &operator=(const Context &other);

public:
    Context(const std::string passw);
    ~Context();

    void registerCommand(const std::string &name, CmdHandler *handler);
    CmdHandler *getCommand(const std::string &name);
    void addNewUser(int sockfd);
    REGISTRED_USERS_MAP::iterator findRegistredUserByFd(int fd);
    void onUserDeconnected(int fd);
    void registerUser(ConnectedUser &user);

    User *getSocketHandler(int sockfd);
    std::string getServerpassw(void);

    void sendClientMsg(User &user, const std::string &msg);
};

#endif
