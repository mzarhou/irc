#include "context.hpp"
#include <unistd.h>

Context::Context(const std::string passw) : serverpassw(passw)
{
    // TODO: add new commands here
    this->registerCommand("USER", new UserCommand(this));
    this->registerCommand("NICK", new NickCommand(this));
    this->registerCommand("PASS", new PassCommand(this));
    this->registerCommand("LIST", new ListCommand(this));
    this->registerCommand("JOIN", new JoinCommand(this));
    this->registerCommand("PART", new PartCommand(this));
    this->registerCommand("MODE", new ModeCommand(this));
    this->registerCommand("PRIVMSG", new PrivMsgCommand(this));
    this->registerCommand("QUIT", new QuitCommand(this));
    this->registerCommand("INVITE", new InviteCommand(this));
    this->registerCommand("TOPIC", new TopicCommand(this));
    this->registerCommand("KICK", new KickCommand(this));
}

Context::~Context()
{
    COMMANDS_MAP::iterator it = commands.begin();
    for (; it != commands.end(); it++)
    {
        delete it->second;
    }
}

void Context::registerCommand(const std::string &name, CmdHandler *handler)
{
    if (getCommand(name))
        throw std::invalid_argument("command already exists");
    commands[name] = handler;
}

void Context::addNewUser(int sockfd)
{
    GuestUser new_user(this, sockfd);
    guest_users[sockfd] = new_user;
}

CmdHandler *Context::getCommand(const std::string &name)
{
    COMMANDS_MAP::const_iterator it = this->commands.find(name);

    if (it == commands.end())
        return nullptr;
    return it->second;
}

User *Context::getSocketHandler(int sockfd)
{
    GUEST_USERS_MAP::iterator guest_pos = guest_users.find(sockfd);
    if (guest_pos != guest_users.end())
    {
        return &guest_pos->second;
    }
    REGISTRED_USERS_MAP::iterator registred_pos = registred_users.find(sockfd);
    if (registred_pos != registred_users.end())
    {
        return &registred_pos->second;
    }
    throw std::invalid_argument("invalid socket file descriptor");
}

RegistredUser *Context::findRegistredUserByNickname(const std::string &nickname)
{
    REGISTRED_USERS_MAP::iterator it = registred_users.begin();
    for (; it != registred_users.end(); it++)
    {
        if (it->second.nickname == nickname)
            return &it->second;
    }
    return nullptr;
}

GuestUser *Context::findGuestUserByNickName(const std::string &nickname)
{
    GUEST_USERS_MAP::iterator it = guest_users.begin();
    for (; it != guest_users.end(); it++)
    {
        if (it->second.nickname == nickname)
            return &it->second;
    }
    return nullptr;
}

bool Context::isNickNameRegistred(const std::string &nickname)
{
    RegistredUser *user = findRegistredUserByNickname(nickname);
    return (user != nullptr);
}

bool Context::isNickNameGuest(const std::string &nickname)
{
    GuestUser *user = findGuestUserByNickName(nickname);
    return (user != nullptr);
}

bool Context::isUserRegistred(const User &user)
{
    REGISTRED_USERS_MAP::const_iterator it = registred_users.find(user.fd);
    return (it != registred_users.end());
}

bool Context::isUserGuest(const User &user)
{
    GUEST_USERS_MAP::const_iterator it = guest_users.find(user.fd);
    return (it != guest_users.end());
}

void Context::disconnectUser(int fd)
{
    /**
     * kick user from all channels
     */
    User *user = getSocketHandler(fd);
    std::vector<Channel *> channels = user->channels();
    std::vector<Channel *>::iterator it = channels.begin();
    for (; it != channels.end(); it++)
    {
        (*it)->kickUser(*user);
    }

    guest_users.erase(fd);
    registred_users.erase(fd);
    close(fd);
}

void Context::disconnectUser(const std::string &nickname)
{
    GuestUser *guest_user = findGuestUserByNickName(nickname);
    if (guest_user)
    {
        disconnectUser(guest_user->fd);
    }
    else
    {
        RegistredUser *registred_user_it = findRegistredUserByNickname(nickname);
        if (registred_user_it)
        {
            disconnectUser(registred_user_it->fd);
        }
    }
}

void Context::registerUser(GuestUser &user)
{
    std::cout << "registring new user " << user.nickname << std::endl;
    guest_users.erase(user.fd);

    RegistredUser ruser(this, user.fd);
    ruser.setNickname(user.nickname);
    ruser.setRealname(user.realname);
    ruser.setUsername(user.username);
    ruser.setPassword(user.password);

    registred_users[user.fd] = ruser;
}

std::string Context::getServerpassw(void)
{
    return (this->serverpassw);
}

Channel &Context::createNewChannel(const std::string &tag)
{
    CHANNELS_MAP::iterator it = channels.find(tag);
    if (it != channels.end())
        return it->second;

    Channel c(this, tag);
    channels[tag] = c;
    CHANNELS_MAP::iterator it2 = channels.find(tag);
    if (it2 == channels.end())
    {
        std::ostringstream oss;
        oss << "UNKOWN error occured when creating channel " << tag << std::endl;
        throw std::invalid_argument(oss.str());
    }
    return it2->second;
}

void Context::joinUserToChannel(User &user, const std::string &tag)
{
    REGISTRED_USERS_MAP::iterator it = registred_users.find(user.fd);
    if (it == registred_users.end())
        throw std::invalid_argument("User is not registred");
    Channel &ch = createNewChannel(tag);
    ch.addNewUser(it->second);
}

std::vector<Channel *> Context::getUserChannels(const User &user)
{
    std::vector<Channel *> userChannels;

    CHANNELS_MAP::iterator it = channels.begin();
    for (; it != channels.end(); it++)
    {
        if (it->second.hasUser(user))
            userChannels.push_back(&it->second);
    }
    return userChannels;
}

bool Context::isChannelExist(const std::string &tag)
{
    CHANNELS_MAP::iterator it = channels.find(tag);
    return (it != channels.end());
}

Channel *Context::getChannel(const std::string &tag)
{
    CHANNELS_MAP::iterator it = channels.find(tag);
    if (it != channels.end())
        return (&it->second);
    return (NULL);
}

void Context::deleteChannel(Channel &ch)
{
    channels.erase(ch.getTag());
}
