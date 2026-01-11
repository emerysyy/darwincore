//
// DirectoryIterator.cpp
// DarwinCore
//

#include <darwincore/foundation/file/DirectoryIterator.h>
#include <chrono>

namespace darwincore {
namespace file {

std::vector<DirectoryEntry> DirectoryIterator::entries() const {
  std::vector<DirectoryEntry> result;
  iterate([&result](const DirectoryEntry &e) { result.push_back(e); });
  return result;
}

std::vector<std::string> DirectoryIterator::files() const {
  std::vector<std::string> result;
  iterate([&result](const DirectoryEntry &e) {
    if (e.type == EntryType::File)
      result.push_back(e.path);
  });
  return result;
}

std::vector<std::string> DirectoryIterator::directories() const {
  std::vector<std::string> result;
  iterate([&result](const DirectoryEntry &e) {
    if (e.type == EntryType::Directory)
      result.push_back(e.path);
  });
  return result;
}

std::vector<std::string>
DirectoryIterator::filesWithExtension(const std::string &ext) const {
  std::vector<std::string> result;
  iterate([&result, &ext](const DirectoryEntry &e) {
    if (e.type == EntryType::File) {
      fs::path p(e.path);
      if (p.extension() == ext || p.extension() == ("." + ext)) {
        result.push_back(e.path);
      }
    }
  });
  return result;
}

void DirectoryIterator::forEach(
    std::function<void(const DirectoryEntry &)> callback) const {
  iterate(std::move(callback));
}

size_t DirectoryIterator::totalSize() const {
  size_t total = 0;
  iterate([&total](const DirectoryEntry &e) {
    if (e.type == EntryType::File)
      total += e.size;
  });
  return total;
}

size_t DirectoryIterator::fileCount() const {
  size_t count = 0;
  iterate([&count](const DirectoryEntry &e) {
    if (e.type == EntryType::File)
      ++count;
  });
  return count;
}

void DirectoryIterator::iterate(
    std::function<void(const DirectoryEntry &)> callback) const {
  try {
    if (recursive_) {
      auto options = fs::directory_options::none;
      if (followSymlinks_)
        options |= fs::directory_options::follow_directory_symlink;

      for (const auto &entry :
           fs::recursive_directory_iterator(rootPath_, options)) {
        if (maxDepth_ >= 0) {
          int depth =
              std::distance(fs::path(rootPath_).begin(), entry.path().begin());
          if (depth > maxDepth_)
            continue;
        }
        processEntry(entry, callback);
      }
    } else {
      for (const auto &entry : fs::directory_iterator(rootPath_)) {
        processEntry(entry, callback);
      }
    }
  } catch (const fs::filesystem_error &) {
    // 忽略权限错误等
  }
}

void DirectoryIterator::processEntry(
    const fs::directory_entry &entry,
    const std::function<void(const DirectoryEntry &)> &callback) const {
  DirectoryEntry de;
  de.path = entry.path().string();
  de.name = entry.path().filename().string();

  if (entry.is_regular_file())
    de.type = EntryType::File;
  else if (entry.is_directory())
    de.type = EntryType::Directory;
  else if (entry.is_symlink())
    de.type = EntryType::Symlink;
  else
    de.type = EntryType::Other;

  de.size = (de.type == EntryType::File) ? entry.file_size() : 0;

  auto ftime = entry.last_write_time();
  de.modTime = std::chrono::system_clock::to_time_t(
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          ftime - fs::file_time_type::clock::now() +
          std::chrono::system_clock::now()));

  if (!filter_ || filter_(de)) {
    callback(de);
  }
}

} // namespace file
} // namespace darwincore
