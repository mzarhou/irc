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
    REGISTRED_USERS_MAP operators;

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
    bool isUserOp(const User &user) const;

    // channel specific modes
    void toggleInviteOnlyStatus(char sign);
    void toggleModeratedStatus(char sign);
    void toggleNoExternalMsgStatus(char sign);
    void toggleOpsOnlyCanChangeTopicStatus(char sign);
    void setLimit(const std::string &limit);

    // user specific modes
    void toggleUserBanStatus(char sign, const std::string &targetNickname);
    void toggleUserVoicedStatus(char sign, const std::string &targetNickname);
    void toggleUserOpStatus(char sign, const std::string &targetNickname);

    void broadcast(const std::string &message);
    void emit(const User &userToExclude, const std::string &message);
    // void broadcast_msg(User &user, const std::string &message);
};

#endif
