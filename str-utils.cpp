#ifndef STR_UTILS_HPP
#define STR_UTILS_HPP

#include <string>
#include <sstream>

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

std::string getFirstWord(const char *message)
{
    std::string word;
    std::istringstream iss(message);
    iss >> word;
    return word;
}

std::pair<std::string, std::string> split(const std::string &str, char splitBy)
{
    size_t npos = str.find_first_of(splitBy);
    if (npos == std::string::npos)
        return std::make_pair(str, "");
    return std::make_pair(str.substr(0, npos), trim(str.substr(npos)));
}

void removeChar(std::string &str, char c)
{
    str.erase(remove(str.begin(), str.end(), c), str.end());
}

bool strIncludes(const std::string &str, char c)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        if (str[i] == c)
            return true;
    }
    return false;
}

#endif
