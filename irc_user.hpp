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
public:
    Context *context;
    int fd;
    std::string nickname;
    std::string login;

public: // constructors
    User(Context *context, int sockfd);
    User(const User &other);
    // User &operator=(const User &other);
    virtual ~User();

public:
    virtual void handleSocket(const Command &cmd) = 0;

public: // static
    static Command parseIntoCmd(std::string &message);
};

class ConnectedUser : public User
{
public:
    std::string password;
    std::string realname;

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
