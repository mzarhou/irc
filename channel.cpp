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

bool Channel::isUserInvited(const User &user) const
{
    REGISTRED_USERS_MAP::const_iterator it = invitedUsers.find(user.fd);
    return (it != invitedUsers.end());
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

void Channel::inviteUser(RegistredUser &user)
{
    invitedUsers[user.fd] = user;
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
    invitedUsers.erase(user.fd);
    if (this->empty())
        context->deleteChannel(*this);
    std::cout << "channel " << tag << ": " << users.size() << std::endl;
}

REGISTRED_USERS_MAP Channel::getUsers()
{
    return this->users;
}

std::string Channel::getKey() const
{
    return this->key;
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
    return strIncludes(modesWithArgs, 'l');
}

bool Channel::requireAuth() const
{
    return strIncludes(modesWithArgs, 'k');
}

std::string Channel::getModes(const User &user) const
{
    std::ostringstream oss;
    oss << '+' << modes << modesWithArgs;
    if (!this->hasUser(user))
        return (oss.str());
    if (this->isLimited())
        oss << ' ' << limit;
    if (this->requireAuth())
        oss << ' ' << key;
    return (oss.str());
}

// channel specific modes
bool Channel::toggleChar(std::string &modes, char sign, char mode)
{
    if (sign == '+' && strIncludes(modes, mode))
        return false;
    if (sign == '-' && !strIncludes(modes, mode))
        return false;

    if (sign == '+')
    {
        size_t pos = 0;
        for (; pos < modes.length(); pos++)
        {
            if (modes[pos] > mode)
                break;
        }
        modes.insert(pos, 1, mode);
    }
    else
    {
        removeChar(modes, mode);
    }
    return true;
}

void Channel::toggleMode(const User &user, char sign, char mode)
{
    if (!this->toggleChar(modes, sign, mode))
    {
        return;
    }
    std::ostringstream oss;
    oss << user.getMsgPrefix() << " MODE " << this->getTag() << " " << sign << mode << std::endl;
    this->broadcast(oss.str());
}

void Channel::toggleModeWithArgs(const User &user, char sign, char mode, bool isSameArg)
{
    bool isToggled = this->toggleChar(modesWithArgs, sign, mode);
    std::ostringstream oss;
    oss << user.getMsgPrefix() << " MODE " << this->getTag() << " " << sign << mode << " ";
    if (mode == 'l')
        oss << limit;
    if (mode == 'k')
        oss << key;
    oss << std::endl;
    if (isToggled || !isSameArg)
        this->broadcast(oss.str());
}

void Channel::toggleLimit(const User &user, char sign, const std::string &limitStr)
{
    if (sign == '-')
        return this->toggleModeWithArgs(user, sign, 'l', true);
    try
    {
        int limit = std::stoi(limitStr);
        if (limit <= 0)
            throw std::invalid_argument("invalid limit number");
        bool isSameArg = ((int)this->limit == limit);
        this->limit = limit;
        this->toggleModeWithArgs(user, sign, 'l', isSameArg);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void Channel::toggleKey(const User &user, char sign, const std::string &validKey)
{
    if (sign == '-' && !this->requireAuth())
        return;
    if (sign == '+')
    {
        this->toggleModeWithArgs(user, sign, 'k', key == validKey);
        key = validKey;
    }
    else if (validKey == key)
    {
        this->toggleModeWithArgs(user, sign, 'k', true);
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
