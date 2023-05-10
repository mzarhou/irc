#ifndef CHANNEL_HPP
#define CHANNEL_HPP

class Context;

#include "context.hpp"

class Channel
{
private:
    Context *context;

public:
    Channel(Context *context);
    ~Channel();
};

#endif
