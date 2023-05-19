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
    std::string topic;

    // channel modes
    std::string modes;
    std::string modesWithArgs;
    std::vector<std::string> bannedNicknames;
    REGISTRED_USERS_MAP voicedUsers;
    REGISTRED_USERS_MAP operators;
    REGISTRED_USERS_MAP invitedUsers;
    size_t limit;
    std::string key;

private:
    bool empty();
    void broadcastToggleUserMessage(const User &user, char sign, const std::string &targetNickname, char mode);
    bool toggleChar(std::string &modes, char sign, char mode);
    void toggleModeWithArgs(const User &user, char sign, char mode, bool isSameArg);

public:
    Channel();
    Channel(Context *context, const std::string &tag);
    ~Channel();

    Channel &operator=(const Channel &other);

    bool isUserOp(const User &user) const;
    bool isUserBanned(const User &user) const;
    bool isUserVoiced(const User &user) const;
    bool isUserInvited(const User &user) const;
    std::string getUsersStr();

    std::string getKey() const;
    std::string getTopic() const;
    void setTopic(const std::string &str);
    bool hasTopic();

    void addNewUser(RegistredUser &user);
    void inviteUser(RegistredUser &user);
    std::string getTag() const;
    bool hasUser(const User &user) const;
    void kickUser(const User &user);
    REGISTRED_USERS_MAP getUsers();
    bool checkLimit() const;

    // check modes
    bool isInviteOnly() const;
    bool everyOneCanChangeTopic() const;
    bool externalMsgsAllowed() const;
    bool moderated() const;
    bool isLimited() const;
    bool requireAuth() const;

    // channel specific modes
    std::string getModes(const User &user) const;
    void toggleMode(const User &user, char sign, char mode);
    void toggleLimit(const User &user, char sign, const std::string &limit);
    void toggleKey(const User &user, char sign, const std::string &key);

    // user specific modes
    void toggleUserBanStatus(const User &user, char sign, const std::string &targetNickname);
    void toggleUserVoicedStatus(const User &user, char sign, const std::string &targetNickname);
    void toggleUserOpStatus(const User &user, char sign, const std::string &targetNickname);

    void broadcast(const std::string &message);
    void emit(const User &userToExclude, const std::string &message);
};

#endif
