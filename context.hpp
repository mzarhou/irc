#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <iostream>
#include <map>

class User;
class GuestUser;
class RegistredUser;
class CmdHandler;
class Channel;

#include "command.hpp"
#include "irc_user.hpp"
#include "channel.hpp"

typedef std::map<int, GuestUser> GUEST_USERS_MAP;
typedef std::map<int, RegistredUser> REGISTRED_USERS_MAP;
typedef std::map<std::string, CmdHandler *> COMMANDS_MAP;
typedef std::map<std::string, Channel> CHANNELS_MAP;

class Context
{
private:
    const std::string serverpassw;
    GUEST_USERS_MAP guest_users;
    REGISTRED_USERS_MAP registred_users;
    COMMANDS_MAP commands;
    CHANNELS_MAP channels;

    // one context only should exists
    Context &operator=(const Context &other);

public:
    Context(const std::string passw);
    ~Context();

    void registerCommand(const std::string &name, CmdHandler *handler);
    CmdHandler *getCommand(const std::string &name);

    void addNewUser(int sockfd);
    REGISTRED_USERS_MAP::iterator findRegistredUserByNickname(const std::string &nickname);
    GUEST_USERS_MAP::iterator findGuestUserByNickName(const std::string &nickname);
    bool isNickNameRegistred(const std::string &nickname);
    bool isUserRegistred(const User &user);
    bool isUserGuest(const User &user);
    bool isNickNameGuest(const std::string &nickname);
    void disconnectUser(int fd);
    void disconnectUser(const std::string &nickname);
    void registerUser(GuestUser &user);

    User *getSocketHandler(int sockfd);
    std::string getServerpassw(void);

    Channel &createNewChannel(const std::string &tag);
    void joinUserToChannel(User &user, const std::string &tag);
    void kickUserFromAllChannels(User &user);
    bool isChannelExist(const std::string &tag);
    Channel *getChannel(const std::string &tag);
};

#endif
