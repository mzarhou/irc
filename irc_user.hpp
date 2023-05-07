#ifndef IRC_USER_HPP
#define IRC_USER_HPP

#include <iostream>
#include <string>

class User
{
public:
    int fd;
    std::string nickname;
    std::string login;

public:
    virtual void handleMessage(const std::string &message) = 0;

public: // static
    static std::string upperFirstWord(const char *message)
    {
        std::string str(message);
        str.erase(str.find_last_not_of("\n") + 1);
        str.erase(str.find_last_not_of("\r") + 1);
        for (int i = 0; i < str.length() && str[i] != ' '; i++)
        {
            str[i] = toupper(str[i]);
        }
        return str;
    }
};

class ConnectedUser : public User
{
public:
    std::string password;
    std::string realname;

    void handleMessage(const std::string &message)
    {
        std::cout << "initial user: " << message << std::endl;
    }
};

class RegistredUser : public User
{
public:
    void handleMessage(const std::string &message)
    {
        std::cout << "irc user: " << message << std::endl;
    }
};

#endif
