#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <string>

struct Command
{
    std::string cmd;
    std::string args;

    bool isValid() const
    {
        // TODO: add other commands here
        return (
            cmd == "NICK" || cmd == "PASS" || cmd == "USER");
    }
};

#endif
