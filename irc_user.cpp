#include "irc_user.hpp"

User::User(Context *context, int sockfd) : context(context), fd(sockfd), nickname(""), login(""){};

User::User(const User &other)
{
    *this = other;
}

// User &User::operator=(const User &other)
// {
//     this->context = other.context;
//     this->fd = other.fd;
//     this->nickname = other.nickname;
//     this->login = other.login;
//     return *this;
// }

User::~User() {}

Command User::parseIntoCmd(std::string &message)
{
    Command cmd;

    message = trim(message);
    message = upperFirstWord(message.c_str());
    std::istringstream ss(message);
    size_t npos = message.find_first_of(" \t\n\r\v\f");
    if (npos == std::string::npos)
        return (Command){message, ""};
    cmd.name = message.substr(0, npos);
    cmd.args = trim(message.substr(npos));
    return cmd;
}

/**
 * ConnectedUser
 */
ConnectedUser::ConnectedUser() : User(NULL, -1) {}
ConnectedUser::ConnectedUser(Context *context, int sockfd) : User(context, sockfd), password(""), realname("") {}
ConnectedUser::ConnectedUser(const ConnectedUser &other) : User(other)
{
    *this = other;
}

// ConnectedUser &ConnectedUser::operator=(const ConnectedUser &other)
// {
//     this->password = other.password;
//     this->realname = other.realname;
//     return *this;
// }

ConnectedUser::~ConnectedUser() {}

void ConnectedUser::handleSocket(const Command &cmd)
{
    CmdHandler *command = context->getCommand(cmd.name);
    if (!command)
    {
        // throw std::invalid_argument("invalid command");
    }
    else
    {
        command->validate(*this, cmd.args);
        command->run(*this, cmd.args);
        onChange();
    }
}

void ConnectedUser::onChange()
{
    if (nickname.empty() || login.empty() || password.empty())
    {
        return;
    }
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
// RegistredUser &RegistredUser::operator=(const RegistredUser &other)
// {
//     (void)other;
//     return *this;
// }
RegistredUser::~RegistredUser()
{
}

void RegistredUser::handleSocket(const Command &cmd)
{
    (void)cmd;
}