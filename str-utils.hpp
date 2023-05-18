#ifndef STR_UTILS_HPP
#define STR_UTILS_HPP

#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <utility>

std::string ltrim(const std::string &str);
std::string rtrim(const std::string &str);
std::string trim(const std::string &str);
std::string upperFirstWord(const char *message);
std::string getFirstWord(const char *message);
std::pair<std::string, std::string> split(const std::string &str, char splitBy);
std::queue<std::string> splitChunks(const std::string &str, char splitBy);
void removeChar(std::string &str, char c);
bool strIncludes(const std::string &str, char c);

#endif
