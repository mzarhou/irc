#ifndef CHANNEL_HPP
#define CHANNEL_HPP

class Context;

#include "context.hpp"

typedef std::map<int, RegistredUser> REGISTRED_USERS_MAP;

class Channel
{
private:
    Context *context;
    std::string tag;
    REGISTRED_USERS_MAP users;

public:
    Channel();
    Channel(Context *context, const std::string &tag);
    ~Channel();
    std::string getTag();

    Channel &operator=(const Channel &other);
};

#endif
