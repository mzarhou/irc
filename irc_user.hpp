#ifndef IRC_USER_HPP
#define IRC_USER_HPP

#include <iostream>
#include <string>
#include <sstream>
#include "command.hpp"
#include "str-utils.hpp"

class User
{
public:
    int fd;
    std::string nickname;
    std::string login;

public: // constructors
    User(int sockfd) : fd(sockfd), nickname(""), login(""){};
    User(const User &other)
    {
        *this = other;
    }
    User &operator=(const User &other)
    {
        this->fd = other.fd;
        this->nickname = other.nickname;
        this->login = other.login;
        return *this;
    }
    ~User() {}

public:
    virtual void handleSocket(const Command &cmd) = 0;

public: // static
    static Command parseIntoCmd(std::string &message)
    {
        Command cmd;

        message = trim(message);
        message = upperFirstWord(message.c_str());
        std::istringstream ss(message);
        size_t npos = message.find_first_of(" \t\n\r\v\f");
        if (npos == std::string::npos)
            return (Command){message, ""};
        cmd.cmd = message.substr(0, npos);
        cmd.args = trim(message.substr(npos));
        return cmd;
    }
};

class ConnectedUser : public User
{
public:
    std::string password;
    std::string realname;

public: // constructors
    ConnectedUser() : User(-1) {}
    ConnectedUser(int sockfd) : User(sockfd), password(""), realname("") {}

    ConnectedUser(const ConnectedUser &other) : User(other)
    {
        *this = other;
    }

    ConnectedUser &operator=(const ConnectedUser &other)
    {
        this->password = other.password;
        this->realname = other.realname;
        return *this;
    }

    ~ConnectedUser() {}

public:
    typedef void (ConnectedUser::*CMD_FUNC)(const std::string &args);

    void handleSocket(const Command &cmd)
    {
        CMD_FUNC command = selectCommand(cmd);

        if (command)
        {
            (this->*command)(cmd.args);
            onChange();
        }
        /**
         * if its a valid command send this message:
         * :punch.wa.us.dal.net 451 * join :You must finish connecting with another nickname first.
         * otherwise ignore it
         */
        else if (cmd.isValid())
        {
            std::ostringstream buffer;
            buffer << ":localhost 451 * " << cmd.cmd << " :You must finish connecting with another nickname first.\r\n";
            std::cout << buffer.str() << std::endl;
        }
    }

    CMD_FUNC selectCommand(const Command &cmd)
    {
        if (cmd.cmd == "NICK")
            return &ConnectedUser::nickCmd;
        else if (cmd.cmd == "PASS")
            return &ConnectedUser::passCmd;
        else if (cmd.cmd == "USER")
            return &ConnectedUser::userCmd;
        return NULL;
    }

    void nickCmd(const std::string &args)
    {
        std::cout << "this is nick command\n";
    }

    void passCmd(const std::string &args)
    {
        std::cout << "this is pass command\n";
    }

    void userCmd(const std::string &args)
    {
        std::cout << "this is user command\n";
    }

    void onChange()
    {
        if (nickname.length() == 0 || login.length() == 0 || password.length() == 0)
        {
            return;
        }
        // register user
    }
};

class RegistredUser : public User
{
public:
    RegistredUser() : User(-1) {}

    RegistredUser(int sockfd) : User(sockfd) {}

    RegistredUser(const RegistredUser &other) : User(other)
    {
        *this = other;
    }

    RegistredUser &operator=(const RegistredUser &other)
    {
        return *this;
    }
    ~RegistredUser()
    {
    }

    void handleSocket(const Command &cmd)
    {
    }
};

#endif
