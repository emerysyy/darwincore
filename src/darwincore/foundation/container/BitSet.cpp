//
// BitSet.cpp
// DarwinCore
//

#include <darwincore/foundation/container/BitSet.h>
#include <algorithm>
#include <cstddef>
#include <string>

namespace darwincore {
namespace container {

BitSet BitSet::operator&(const BitSet &other) const {
  BitSet result(std::max(size_, other.size_));
  size_t minWords = std::min(bits_.size(), other.bits_.size());
  for (size_t i = 0; i < minWords; ++i)
    result.bits_[i] = bits_[i] & other.bits_[i];
  return result;
}

BitSet BitSet::operator|(const BitSet &other) const {
  BitSet result(std::max(size_, other.size_));
  for (size_t i = 0; i < result.bits_.size(); ++i) {
    uint64_t a = i < bits_.size() ? bits_[i] : 0;
    uint64_t b = i < other.bits_.size() ? other.bits_[i] : 0;
    result.bits_[i] = a | b;
  }
  return result;
}

BitSet BitSet::operator^(const BitSet &other) const {
  BitSet result(std::max(size_, other.size_));
  for (size_t i = 0; i < result.bits_.size(); ++i) {
    uint64_t a = i < bits_.size() ? bits_[i] : 0;
    uint64_t b = i < other.bits_.size() ? other.bits_[i] : 0;
    result.bits_[i] = a ^ b;
  }
  return result;
}

std::string BitSet::toString() const {
  std::string result(size_, '0');
  for (size_t i = 0; i < size_; ++i)
    if (get(i))
      result[size_ - 1 - i] = '1';
  return result;
}

size_t BitSet::findFirst() const {
  for (size_t i = 0; i < bits_.size(); ++i) {
    if (bits_[i]) {
      size_t pos = i * 64 + __builtin_ctzll(bits_[i]);
      return pos < size_ ? pos : size_;
    }
  }
  return size_;
}

} // namespace container
} // namespace darwincore
