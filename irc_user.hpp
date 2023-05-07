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

    void handleSocket(const Command &cmd)
    {
        std::cout << "cmd: " << cmd.cmd << std::endl;
        std::cout << "args: " << cmd.args << std::endl;
        std::cout << "-------" << std::endl;
    }
};

class RegistredUser : public User
{
public:
    void handleSocket(const Command &cmd)
    {
        std::cout << "cmd: " << cmd.cmd << std::endl;
        std::cout << "args: " << cmd.args << std::endl;
        std::cout << "-------" << std::endl;
    }
};

#endif
