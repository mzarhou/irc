#ifndef IRC_USER_HPP
#define IRC_USER_HPP

#include <iostream>
#include <string>
#include <sstream>
#include "str-utils.hpp"

class Context;
class Channel;
struct Command;

#include "command.hpp"
#include "context.hpp"

class User
{
protected:
    Context *context;

public:
    int fd;
    std::string nickname;
    std::string realname;
    std::string username;
    std::string password;

public: // constructors
    User(Context *context, int sockfd);
    User(const User &other);
    // User &operator=(const User &other);
    virtual ~User();

    // policy
    void canJoinChannel(const Channel &ch, const std::string &key);
    void canJoinChannel(const std::string &channelTag, const std::string &key);
    void canSendPrivMessage(const std::string &channelTagOrNickname);
    void canManageChannelModes(const Channel &ch);
    void canManageChannelModes(const std::string &channelTagOrNickname);

    bool isRegistred();
    bool isGuest();
    bool isChannelOp(const Channel &ch);
    bool isChannelOp(const std::string &channelTag);
    void send(const std::string &msg) const;
    void sendToUserChannels(const std::string &message);
    bool isJoinedChannel(const std::string &channelTag);
    std::vector<Channel *> channels();
    std::string getMsgPrefix() const;

    void setNickname(const std::string &value);
    void setRealname(const std::string &value);
    void setUsername(const std::string &value);
    void setPassword(const std::string &value);

public:
    virtual void handleSocket(const Command &cmd) = 0;
};

class GuestUser : public User
{
public: // constructors
    GuestUser();
    GuestUser(Context *context, int sockfd);
    GuestUser(const GuestUser &other);
    // GuestUser &operator=(const GuestUser &other);
    ~GuestUser();

public:
    void handleSocket(const Command &cmd);

private: // commands
    void onChange();
};

class RegistredUser : public User
{
public:
    RegistredUser();
    RegistredUser(Context *context, int sockfd);
    RegistredUser(const RegistredUser &other);
    ~RegistredUser();

    void handleSocket(const Command &cmd);
};

#endif
