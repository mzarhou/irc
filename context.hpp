#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <iostream>
#include <map>

class User;
class ConnectedUser;
class RegistredUser;

#include "irc_user.hpp"

typedef std::map<int, ConnectedUser> CONNECTED_USERS_MAP;
typedef std::map<std::string, RegistredUser> REGISTRED_USERS_MAP;

class Context
{
private:
    const std::string serverpassw;
    CONNECTED_USERS_MAP connected_users;
    REGISTRED_USERS_MAP registred_users;

public:
    Context(const std::string passw);
    void addNewUser(int sockfd);
    User *getSocketHandler(int sockfd);
    REGISTRED_USERS_MAP::iterator findRegistredUserByFd(int fd);
    void onUserDeconnected(int fd);
    void registerUser(ConnectedUser &user);
};

#endif
