//
// BloomFilter.cpp
// DarwinCore
//

#include <darwincore/foundation/algorithm/BloomFilter.h>
#include <stdexcept>

namespace darwincore {
namespace algorithm {

BloomFilter::BloomFilter(size_t expectedElements, double falsePositiveRate)
    : numHashes_(optimalHashCount(expectedElements, falsePositiveRate)),
      bits_(optimalBitCount(expectedElements, falsePositiveRate), false) {}

BloomFilter::BloomFilter(size_t bitCount, size_t hashCount)
    : numHashes_(hashCount), bits_(bitCount, false) {}

void BloomFilter::add(const void *data, size_t length) {
  auto hashes = doubleHash(data, length);
  uint64_t h1 = hashes.first;
  uint64_t h2 = hashes.second;
  for (size_t i = 0; i < numHashes_; ++i) {
    size_t index = (h1 + i * h2) % bits_.size();
    bits_[index] = true;
  }
  ++count_;
}

bool BloomFilter::mightContain(const void *data, size_t length) const {
  auto hashes = doubleHash(data, length);
  uint64_t h1 = hashes.first;
  uint64_t h2 = hashes.second;
  for (size_t i = 0; i < numHashes_; ++i) {
    size_t index = (h1 + i * h2) % bits_.size();
    if (!bits_[index])
      return false;
  }
  return true;
}

double BloomFilter::fillRatio() const {
  size_t setBits = 0;
  for (bool b : bits_)
    if (b)
      ++setBits;
  return static_cast<double>(setBits) / bits_.size();
}

double BloomFilter::estimatedFalsePositiveRate() const {
  double fill = fillRatio();
  return std::pow(fill, static_cast<double>(numHashes_));
}

void BloomFilter::clear() {
  std::fill(bits_.begin(), bits_.end(), false);
  count_ = 0;
}

BloomFilter &BloomFilter::merge(const BloomFilter &other) {
  if (bits_.size() != other.bits_.size() || numHashes_ != other.numHashes_)
    throw std::invalid_argument(
        "Cannot merge BloomFilters with different parameters");
  for (size_t i = 0; i < bits_.size(); ++i)
    bits_[i] = bits_[i] || other.bits_[i];
  count_ += other.count_;
  return *this;
}

size_t BloomFilter::optimalBitCount(size_t n, double p) {
  return static_cast<size_t>(-1.0 * n * std::log(p) /
                             (std::log(2) * std::log(2)));
}

size_t BloomFilter::optimalHashCount(size_t n, double p) {
  size_t m = optimalBitCount(n, p);
  return static_cast<size_t>(static_cast<double>(m) / n * std::log(2));
}

std::pair<uint64_t, uint64_t> BloomFilter::doubleHash(const void *data,
                                                      size_t length) const {
  uint64_t h1 = Hash::fnv1a64(data, length);
  uint64_t h2 = Hash::murmur3_32(data, length, 0x9747b28c);
  return {h1, h2};
}

} // namespace algorithm
} // namespace darwincore
