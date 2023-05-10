#ifndef STR_UTILS_HPP
#define STR_UTILS_HPP

#include <string>

std::string ltrim(const std::string &str);
std::string rtrim(const std::string &str);
std::string trim(const std::string &str);
std::string upperFirstWord(const char *message);
std::string getFirstWord(const char *message);

#endif
