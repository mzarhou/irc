#include "irc_user.hpp"

/**
 * User
 */
User::User(Context *context, int sockfd) : context(context), fd(sockfd), nickname(""), username(""){};

User::User(const User &other)
{
    *this = other;
}

User::~User() {}

void User::setNickname(const std::string &value)
{
    this->nickname = value;
}

void User::setRealname(const std::string &value)
{
    this->realname = value;
}

void User::setUsername(const std::string &value)
{
    this->username = value;
}

void User::setPassword(const std::string &value)
{
    this->password = value;
}

Command User::parseIntoCmd(std::string &message)
{
    Command cmd;

    cmd.originalName = getFirstWord(message.c_str());

    message = trim(message);
    message = upperFirstWord(message.c_str());
    std::istringstream ss(message);
    size_t npos = message.find_first_of(" \t\n\r\v\f");
    if (npos == std::string::npos)
        return (Command){cmd.originalName, message, ""};
    cmd.name = message.substr(0, npos);
    cmd.args = trim(message.substr(npos));
    return cmd;
}

/**
 * ConnectedUser
 */
ConnectedUser::ConnectedUser() : User(NULL, -1) {}
ConnectedUser::ConnectedUser(Context *context, int sockfd) : User(context, sockfd) {}
ConnectedUser::ConnectedUser(const ConnectedUser &other) : User(other)
{
    *this = other;
}
ConnectedUser::~ConnectedUser() {}

void ConnectedUser::handleSocket(const Command &cmd)
{
    CmdHandler *command = context->getCommand(cmd.name);
    if (!command)
    {
        return;
    }

    /**
     * user must register before sending other valid commands
     * than this ones
     */
    std::string cmds[] = {"PASS", "USER", "NICK"};
    std::string *it = std::find(std::begin(cmds), std::end(cmds), cmd.name);
    if (it == std::end(cmds))
    {
        context->sendClientMsg(*this, ":localhost 451 * LIST :You must finish connecting with another nickname first.\n");
    }
    else
    {
        try
        {
            command->validate(*this, cmd.args);
            command->run(*this, cmd.args);
            onChange();
        }
        catch (const std::exception &e)
        {
            context->sendClientMsg(*this, e.what());
        }
    }
}

void ConnectedUser::onChange()
{
    std::cout << "-> socket fd: " << fd << '\n';
    std::cout << "-> nickname: " << nickname << '\n';
    std::cout << "-> username: " << username << '\n';
    std::cout << "-> password: " << password << '\n';
    if (nickname.empty() || username.empty() || password.empty())
        return;

    std::ostringstream oss;
    oss << ":localhost " << nickname << " :Welcome to the 1337.server.chat Internet Relay Chat Network " << nickname << '\n';
    context->sendClientMsg(*this, oss.str());
    context->registerUser(*this);
}

/**
 * RegistredUser
 */
RegistredUser::RegistredUser() : User(NULL, -1) {}
RegistredUser::RegistredUser(Context *context, int sockfd) : User(context, sockfd) {}
RegistredUser::RegistredUser(const RegistredUser &other) : User(other)
{
    *this = other;
}
RegistredUser::~RegistredUser()
{
}

void RegistredUser::handleSocket(const Command &cmd)
{
    CmdHandler *command = context->getCommand(cmd.name);
    if (!command)
    {
        std::ostringstream oss;
        oss << ":localhost 421 " << nickname << " " << cmd.originalName << " :Unknown command\n";
        context->sendClientMsg(*this, oss.str());
        return;
    }

    try
    {
        command->validate(*this, cmd.args);
        command->run(*this, cmd.args);
    }
    catch (const std::exception &e)
    {
        context->sendClientMsg(*this, e.what());
    }
}
