//
// Regex.cpp
// DarwinCore
//

#include <darwincore/foundation/string/Regex.h>

namespace darwincore {
namespace string {

bool Regex::match(const std::string &str) const {
  return std::regex_match(str, regex_);
}

bool Regex::search(const std::string &str) const {
  return std::regex_search(str, regex_);
}

std::optional<RegexMatch> Regex::findFirst(const std::string &str) const {
  std::smatch match;
  if (!std::regex_search(str, match, regex_))
    return std::nullopt;

  RegexMatch result;
  result.value = match.str();
  result.position = match.position();
  result.length = match.length();
  for (size_t i = 1; i < match.size(); ++i)
    result.groups.push_back(match[i].str());
  return result;
}

std::vector<RegexMatch> Regex::findAll(const std::string &str) const {
  std::vector<RegexMatch> results;
  auto begin = std::sregex_iterator(str.begin(), str.end(), regex_);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    RegexMatch result;
    result.value = it->str();
    result.position = it->position();
    result.length = it->length();
    for (size_t i = 1; i < it->size(); ++i)
      result.groups.push_back((*it)[i].str());
    results.push_back(std::move(result));
  }
  return results;
}

std::string Regex::replaceFirst(const std::string &str,
                                const std::string &replacement) const {
  return std::regex_replace(str, regex_, replacement,
                            std::regex_constants::format_first_only);
}

std::string Regex::replaceAll(const std::string &str,
                              const std::string &replacement) const {
  return std::regex_replace(str, regex_, replacement);
}

std::vector<std::string> Regex::split(const std::string &str) const {
  std::vector<std::string> result;
  std::sregex_token_iterator it(str.begin(), str.end(), regex_, -1);
  std::sregex_token_iterator end;
  for (; it != end; ++it)
    result.push_back(*it);
  return result;
}

} // namespace string
} // namespace darwincore
