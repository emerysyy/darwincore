//
// StringPool.cpp
// DarwinCore
//

#include <darwincore/foundation/string/StringPool.h>

namespace darwincore {
namespace string {

std::string_view StringPool::intern(const std::string &str) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto [it, inserted] = pool_.insert(str);
  return *it;
}

std::string_view StringPool::intern(std::string_view str) {
  return intern(std::string(str));
}

bool StringPool::contains(const std::string &str) const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pool_.find(str) != pool_.end();
}

size_t StringPool::size() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return pool_.size();
}

void StringPool::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  pool_.clear();
}

} // namespace string
} // namespace darwincore
