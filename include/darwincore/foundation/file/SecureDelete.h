//
// SecureDelete.h
// DarwinCore
//

#ifndef DARWINCORE_SECURE_DELETE_H
#define DARWINCORE_SECURE_DELETE_H

#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace darwincore {
namespace file {

/// 安全删除模式
enum class SecureDeleteMode {
  Simple, // 单次零覆写
  DoD,    // DoD 5220.22-M（3次覆写）
  Gutmann // Gutmann 方法（35次覆写）
};

/// 安全删除工具
class SecureDelete {
public:
  /**
   * @brief 安全删除文件
   */
  static bool deleteFile(const std::string &path,
                         SecureDeleteMode mode = SecureDeleteMode::DoD);

  /**
   * @brief 安全删除目录（递归）
   */
  static bool deleteDirectory(const std::string &path,
                              SecureDeleteMode mode = SecureDeleteMode::DoD);

  /**
   * @brief 安全清零内存
   */
  static void secureZeroMemory(void *ptr, size_t size);

private:
  /// 零覆写
  static bool overwriteZeros(int fd, size_t size, std::vector<uint8_t> &buffer);

  /// DoD 5220.22-M 覆写
  static bool overwriteDoD(int fd, size_t size, std::vector<uint8_t> &buffer);

  /// Gutmann 35次覆写
  static bool overwriteGutmann(int fd, size_t size,
                               std::vector<uint8_t> &buffer);

  /// 写入固定模式
  static bool overwritePattern(int fd, size_t size,
                               const std::vector<uint8_t> &buffer);

  /// 写入随机数据
  static bool overwriteRandom(int fd, size_t size,
                              std::vector<uint8_t> &buffer);

  /// 生成随机文件名
  static std::string generateRandomName(const std::string &path);
};

} // namespace file
} // namespace darwincore

#endif // DARWINCORE_SECURE_DELETE_H
