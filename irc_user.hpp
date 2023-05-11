#ifndef IRC_USER_HPP
#define IRC_USER_HPP

#include <iostream>
#include <string>
#include <sstream>
#include "str-utils.hpp"

class Context;
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
    void setNickname(const std::string &value);
    void setRealname(const std::string &value);
    void setUsername(const std::string &value);
    void setPassword(const std::string &value);

public:
    virtual void handleSocket(const Command &cmd) = 0;

public: // static
    static Command parseIntoCmd(std::string &message);
};

class ConnectedUser : public User
{
public: // constructors
    ConnectedUser();
    ConnectedUser(Context *context, int sockfd);
    ConnectedUser(const ConnectedUser &other);
    // ConnectedUser &operator=(const ConnectedUser &other);
    ~ConnectedUser();

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
    // RegistredUser &operator=(const RegistredUser &other);
    ~RegistredUser();

    void handleSocket(const Command &cmd);
};

#endif
