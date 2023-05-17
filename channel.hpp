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

    // channel modes
    std::string modes;
    std::vector<std::string> bannedNicknames;
    REGISTRED_USERS_MAP voicedUsers;
    REGISTRED_USERS_MAP operators;
    size_t limit;

private:
    bool empty();

public:
    Channel();
    Channel(Context *context, const std::string &tag);
    ~Channel();

    Channel &operator=(const Channel &other);

    bool isUserOp(const User &user) const;
    std::string getUsersStr();

    void addNewUser(RegistredUser &user);
    std::string getTag();
    bool hasUser(const User &user);
    void kickUser(const User &user);

    // check modes
    bool isInviteOnly();
    bool everyOneCanChangeTopic();
    bool externalMsgsAllowed();
    bool moderated();
    bool isLimited();

    // channel specific modes
    std::string getModes();
    void toggleMode(const User &user, char sign, char mode);
    void toggleLimit(const User &user, char sign, const std::string &limit);

    // user specific modes
    void toggleUserBanStatus(char sign, const std::string &targetNickname);
    void toggleUserVoicedStatus(char sign, const std::string &targetNickname);
    void toggleUserOpStatus(char sign, const std::string &targetNickname);

    void broadcast(const std::string &message);
    void emit(const User &userToExclude, const std::string &message);
};

#endif
