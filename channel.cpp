#include "channel.hpp"

Channel::Channel(Context *context, const std::string &tag) : context(context), tag(tag), modes("nt")
{
}

Channel::Channel() : context(NULL), tag(""), modes("nt")
{
}

Channel::~Channel()
{
    (void)context;
}

std::string Channel::getTag() const
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

bool Channel::isUserBanned(const User &user) const
{
    std::vector<std::string>::const_iterator it = std::find(bannedNicknames.begin(), bannedNicknames.end(), user.nickname);
    return (it != bannedNicknames.end());
}

bool Channel::isUserVoiced(const User &user) const
{
    REGISTRED_USERS_MAP::const_iterator it = voicedUsers.find(user.fd);
    return (it != voicedUsers.end());
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

bool Channel::checkLimit() const
{
    return (!isLimited() || limit > users.size());
}

void Channel::addNewUser(RegistredUser &user)
{
    if (this->empty())
        operators[user.fd] = user;
    users[user.fd] = user;
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

bool Channel::hasUser(const User &user) const
{
    REGISTRED_USERS_MAP::const_iterator it = users.find(user.fd);
    return (it != users.end());
}

void Channel::kickUser(const User &user)
{
    users.erase(user.fd);
    operators.erase(user.fd);
    voicedUsers.erase(user.fd);
    if (this->empty())
        context->deleteChannel(*this);
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

REGISTRED_USERS_MAP Channel::getUsers()
{
    return this->users;
}

// check modes
bool Channel::isInviteOnly() const
{
    return strIncludes(modes, 'i');
}

bool Channel::everyOneCanChangeTopic() const
{
    return !strIncludes(modes, 't');
}

bool Channel::externalMsgsAllowed() const
{
    return !strIncludes(modes, 'n');
}

bool Channel::moderated() const
{
    return strIncludes(modes, 'm');
}

bool Channel::isLimited() const
{
    return strIncludes(modes, 'l');
}

std::string Channel::getModes() const
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

void Channel::broadcastToggleUserMessage(const User &user, char sign, const std::string &targetNickname, char mode)
{
    std::ostringstream oss;

    oss << user.getMsgPrefix() << " MODE " << this->getTag() << " " << sign << mode
        << " " << targetNickname << "!*@*\n";
    this->broadcast(oss.str());
}

void Channel::toggleUserBanStatus(const User &user, char sign, const std::string &targetNickname)
{
    std::vector<std::string>::iterator it = std::find(bannedNicknames.begin(), bannedNicknames.end(), targetNickname);
    if (it == bannedNicknames.end())
        this->broadcastToggleUserMessage(user, sign, targetNickname, 'b');
    if (sign == '+')
    {
        if (it != bannedNicknames.end())
            return;
        bannedNicknames.push_back(targetNickname);
        RegistredUser *user = context->findRegistredUserByNickname(targetNickname);
        if (user)
            this->kickUser(*user);
    }
    else
    {
        bannedNicknames.erase(it);
    }
}

void Channel::toggleUserVoicedStatus(const User &user, char sign, const std::string &targetNickname)
{
    RegistredUser *targetUser = context->findRegistredUserByNickname(targetNickname);
    if (!targetUser)
        return;

    if (sign == '+')
    {
        voicedUsers[targetUser->fd] = *targetUser;
    }
    else
    {
        voicedUsers.erase(targetUser->fd);
    }
    this->broadcastToggleUserMessage(user, sign, targetNickname, 'v');
}

void Channel::toggleUserOpStatus(const User &user, char sign, const std::string &targetNickname)
{
    RegistredUser *targetUser = context->findRegistredUserByNickname(targetNickname);
    if (!targetUser)
        return;

    if (sign == '+')
    {
        operators[targetUser->fd] = *targetUser;
    }
    else
    {
        operators.erase(targetUser->fd);
    }
    this->broadcastToggleUserMessage(user, sign, targetNickname, 'o');
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
