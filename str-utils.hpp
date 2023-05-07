#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

std::string ltrim(const std::string &str)
{
    size_t npos = str.find_first_not_of(" \t\n\r\v\f");
    if (npos == std::string::npos)
        return std::string("");
    return str.substr(npos);
}

std::string rtrim(const std::string &str)
{
    size_t npos = str.find_last_not_of(" \t\n\r\v\f");
    if (npos == std::string::npos)
        return std::string("");
    return str.substr(0, npos + 1);
}

std::string trim(const std::string &str)
{
    return ltrim(rtrim(str));
}

std::string upperFirstWord(const char *message)
{
    std::string str(message);
    // str.erase(str.find_last_not_of("\n") + 1);
    // str.erase(str.find_last_not_of("\r") + 1);
    for (size_t i = 0; i < str.length() && str[i] != ' '; i++)
    {
        str[i] = toupper(str[i]);
    }
    return str;
}

#endif
