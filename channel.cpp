#include "channel.hpp"

Channel::Channel(Context *context) : context(context)
{
}

Channel::~Channel()
{
    (void)context;
}
