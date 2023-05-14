#ifndef ERRORS_HPP
#define ERRORS_HPP

#define RPL_TOPIC 332
#define RPL_TOPICWHOTIME 333
#define RPL_NAMREPLY 353
#define RPL_ENDOFNAMES 366
#define ERR_TOOMANYCHANNELS 405
#define ERR_CHANNELISFULL 471
#define ERR_INVITEONLYCHAN 473
#define ERR_BANNEDFROMCHAN 474
#define ERR_BADCHANNELKEY 475
#define ERR_BADCHANMASK 476

#include <string>
#include <sstream>

struct Error
{
    static std::string ERR_NEEDMOREPARAMS(const std::string &serverHost, const std::string &nickname)
    {
        std::ostringstream oss;
        oss << ":" << serverHost << " 461 " << nickname << " JOIN :Not enough parameters\n";
        return oss.str();
    }
    static std::string ERR_NOSUCHCHANNEL(const std::string &serverHost, const std::string &nickname, const std::string &channel)
    {
        std::ostringstream oss;
        oss << ":" << serverHost << " 403 " << nickname << " " << channel << " :No such channel\n";
        return oss.str();
    }

    static std::string ERR_CHANOPRIVSNEEDED(const std::string &serverHost, const std::string &nickname, const std::string &channel)
    {
        std::ostringstream oss;
        oss << ":" << serverHost << " 482 " << nickname << " " << channel << " :You're not a channel operator\n";
        return oss.str();
    }
};

#endif
