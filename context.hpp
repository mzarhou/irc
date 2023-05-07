#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <iostream>
#include <map>
#include "irc_user.hpp"

typedef std::map<int, ConnectedUser> CONNECTED_USERS_MAP;
typedef std::map<std::string, RegistredUser> REGISTRED_USERS_MAP;

class Context
{
private:
    CONNECTED_USERS_MAP connected_users;
    REGISTRED_USERS_MAP registred_users;

public:
    void addNewUser(int sockfd)
    {
        ConnectedUser new_user;
        connected_users[sockfd] = new_user;
    }

    User *getSocketHandler(int sockfd)
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

    REGISTRED_USERS_MAP::iterator findRegistredUserByFd(int fd)
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

    void onUserDeconnected(int fd)
    {
        connected_users.erase(fd);

        REGISTRED_USERS_MAP::iterator registred_pos = findRegistredUserByFd(fd);
        if (registred_pos != registred_users.end())
        {
            registred_users.erase(registred_pos);
        }
    }
};

#endif
