//
// UUID.cpp
// DarwinCore
//

#include <darwincore/foundation/uuid/UUID.h>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace darwincore {
namespace uuid {

UUID UUID::generate() {
  UUID uuid;

  thread_local std::random_device rd;
  thread_local std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;

  uint64_t hi = dist(gen);
  uint64_t lo = dist(gen);

  // 设置版本 4（随机）
  hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
  // 设置变体（RFC 4122）
  lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

  uuid.data_[0] = (hi >> 56) & 0xFF;
  uuid.data_[1] = (hi >> 48) & 0xFF;
  uuid.data_[2] = (hi >> 40) & 0xFF;
  uuid.data_[3] = (hi >> 32) & 0xFF;
  uuid.data_[4] = (hi >> 24) & 0xFF;
  uuid.data_[5] = (hi >> 16) & 0xFF;
  uuid.data_[6] = (hi >> 8) & 0xFF;
  uuid.data_[7] = hi & 0xFF;
  uuid.data_[8] = (lo >> 56) & 0xFF;
  uuid.data_[9] = (lo >> 48) & 0xFF;
  uuid.data_[10] = (lo >> 40) & 0xFF;
  uuid.data_[11] = (lo >> 32) & 0xFF;
  uuid.data_[12] = (lo >> 24) & 0xFF;
  uuid.data_[13] = (lo >> 16) & 0xFF;
  uuid.data_[14] = (lo >> 8) & 0xFF;
  uuid.data_[15] = lo & 0xFF;

  return uuid;
}

std::optional<UUID> UUID::parse(const std::string &str) {
  UUID uuid;
  std::string clean;

  for (char c : str) {
    if (c != '-')
      clean += c;
  }

  if (clean.size() != 32)
    return std::nullopt;

  for (size_t i = 0; i < 16; ++i) {
    int value;
    try {
      value = std::stoi(clean.substr(i * 2, 2), nullptr, 16);
    } catch (...) {
      return std::nullopt;
    }
    uuid.data_[i] = static_cast<uint8_t>(value);
  }

  return uuid;
}

UUID UUID::nil() { return UUID(); }

UUID::UUID() : data_{} {}

std::string UUID::toString() const {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (int i = 0; i < 4; ++i)
    oss << std::setw(2) << static_cast<int>(data_[i]);
  oss << '-';
  for (int i = 4; i < 6; ++i)
    oss << std::setw(2) << static_cast<int>(data_[i]);
  oss << '-';
  for (int i = 6; i < 8; ++i)
    oss << std::setw(2) << static_cast<int>(data_[i]);
  oss << '-';
  for (int i = 8; i < 10; ++i)
    oss << std::setw(2) << static_cast<int>(data_[i]);
  oss << '-';
  for (int i = 10; i < 16; ++i)
    oss << std::setw(2) << static_cast<int>(data_[i]);

  return oss.str();
}

std::string UUID::toCompactString() const {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (uint8_t byte : data_)
    oss << std::setw(2) << static_cast<int>(byte);
  return oss.str();
}

int UUID::variant() const {
  if ((data_[8] & 0x80) == 0x00)
    return 0;
  if ((data_[8] & 0xC0) == 0x80)
    return 1;
  if ((data_[8] & 0xE0) == 0xC0)
    return 2;
  return 3;
}

bool UUID::isNil() const {
  for (uint8_t byte : data_)
    if (byte != 0)
      return false;
  return true;
}

size_t UUID::hash() const {
  size_t h = 0;
  for (uint8_t byte : data_)
    h = h * 31 + byte;
  return h;
}

} // namespace uuid
} // namespace darwincore
