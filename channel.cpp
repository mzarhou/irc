#include "channel.hpp"

Channel::Channel(Context *context, const std::string &tag) : context(context), tag(tag)
{
}

Channel::Channel() : context(NULL), tag("")
{
}

Channel::~Channel()
{
    (void)context;
}

std::string Channel::getTag()
{
    return this->tag;
}

Channel &Channel::operator=(const Channel &other)
{
    this->context = other.context;
    this->tag = other.tag;
    this->users = other.users;
    return *this;
}

bool Channel::isUserOp(const User &user) const
{
    REGISTRED_USERS_MAP::const_iterator opsIt = operators.find(user.fd);
    return (opsIt != operators.end());
}

std::string Channel::getUsersStr()
{
    std::string usersStr = "";
    REGISTRED_USERS_MAP::iterator usersIt = users.begin();
    for (; usersIt != users.end(); usersIt++)
    {
        if (!this->isUserOp(usersIt->second))
        {
            usersStr += usersIt->second.nickname;
            usersStr += " ";
        }
    }
    REGISTRED_USERS_MAP::iterator opsIt = operators.begin();
    for (; opsIt != operators.end(); opsIt++)
    {
        usersStr += "@";
        usersStr += opsIt->second.nickname;
        usersStr += " ";
    }
    return usersStr;
}

void Channel::addNewUser(RegistredUser &user)
{
    if (this->empty())
        operators[user.fd] = user;
    users[user.fd] = user;
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

bool Channel::hasUser(const User &user)
{
    REGISTRED_USERS_MAP::iterator it = users.find(user.fd);
    return (it != users.end());
}

void Channel::kickUser(const User &user)
{
    users.erase(user.fd);
    operators.erase(user.fd);
    if (this->empty())
        context->deleteChannel(*this);
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

// check modes
bool Channel::isInviteOnly()
{
    return strIncludes(modes, 'i');
}

bool Channel::everyOneCanChangeTopic()
{
    return !strIncludes(modes, 't');
}

bool Channel::externalMsgsAllowed()
{
    return !strIncludes(modes, 'n');
}

bool Channel::moderated()
{
    return strIncludes(modes, 'm');
}

bool Channel::isLimited()
{
    return strIncludes(modes, 'l');
}

std::string Channel::getModes()
{
    std::ostringstream oss;
    oss << '+' << modes;
    if (this->isLimited())
        oss << ' ' << limit;
    return (oss.str());
}

// channel specific modes
void Channel::toggleMode(const User &user, char sign, char mode)
{
    if (sign == '+' && strIncludes(modes, mode))
        return;
    if (sign == '-' && !strIncludes(modes, mode))
        return;

    if (sign == '+' && mode == 'l')
        modes += 'l';
    else if (sign == '+')
    {
        size_t pos = 0;
        for (; pos < modes.length(); pos++)
        {
            if (modes[pos] == 'l' || modes[pos] > mode)
                break;
        }
        modes.insert(pos, 1, mode);
    }
    else
    {
        removeChar(modes, mode);
    }

    std::ostringstream oss;
    oss << user.getMsgPrefix() << " MODE " << this->getTag() << " " << sign << mode;
    if (mode == 'l')
        oss << " " << limit;
    oss << std::endl;
    this->broadcast(oss.str());
}

void Channel::toggleLimit(const User &user, char sign, const std::string &limitStr)
{
    if (sign == '-')
        return this->toggleMode(user, sign, 'l');
    try
    {
        int limit = std::stoi(limitStr);
        if (limit <= 0)
            return;
        this->limit = limit;
        this->toggleMode(user, sign, 'l');
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

// user specific modes
void Channel::toggleUserBanStatus(char sign, const std::string &targetNickname)
{
    (void)sign;
    (void)targetNickname;
}

void Channel::toggleUserVoicedStatus(char sign, const std::string &targetNickname)
{
    (void)sign;
    (void)targetNickname;
}

void Channel::toggleUserOpStatus(char sign, const std::string &targetNickname)
{
    (void)sign;
    (void)targetNickname;
}

/**
 * send message all users in the channel
 */
void Channel::broadcast(const std::string &message)
{
    REGISTRED_USERS_MAP::iterator it = users.begin();
    for (; it != users.end(); it++)
        it->second.send(message);
}

/**
 * send message all users in the channel
 * excluding current authenticated user
 */
void Channel::emit(const User &userToExclude, const std::string &message)
{
    REGISTRED_USERS_MAP::iterator it = users.begin();
    for (; it != users.end(); it++)
    {
        if (it->second.fd == userToExclude.fd)
            continue;
        it->second.send(message);
    }
}

bool Channel::empty()
{
    return (users.begin() == users.end());
}
