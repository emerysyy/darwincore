//
// PathUtils.cpp
// DarwinCore
//

#include <darwincore/foundation/file/PathUtils.h>
#include <cstdlib>

namespace darwincore {
namespace file {

namespace fs = std::filesystem;

std::string PathUtils::join(const std::string &base, const std::string &path) {
  return (fs::path(base) / path).string();
}

std::string PathUtils::fileName(const std::string &path) {
  return fs::path(path).filename().string();
}

std::string PathUtils::baseName(const std::string &path) {
  return fs::path(path).stem().string();
}

std::string PathUtils::extension(const std::string &path) {
  return fs::path(path).extension().string();
}

std::string PathUtils::extensionWithoutDot(const std::string &path) {
  std::string ext = extension(path);
  return ext.empty() ? "" : ext.substr(1);
}

std::string PathUtils::parentPath(const std::string &path) {
  return fs::path(path).parent_path().string();
}

std::string PathUtils::normalize(const std::string &path) {
  return fs::path(path).lexically_normal().string();
}

std::string PathUtils::absolutePath(const std::string &path) {
  return fs::absolute(path).string();
}

std::string PathUtils::relativePath(const std::string &path,
                                    const std::string &base) {
  return fs::relative(path, base).string();
}

bool PathUtils::exists(const std::string &path) { return fs::exists(path); }

bool PathUtils::isAbsolute(const std::string &path) {
  return fs::path(path).is_absolute();
}

bool PathUtils::isRelative(const std::string &path) {
  return fs::path(path).is_relative();
}

bool PathUtils::isDirectory(const std::string &path) {
  return fs::is_directory(path);
}

bool PathUtils::isFile(const std::string &path) {
  return fs::is_regular_file(path);
}

bool PathUtils::isSymlink(const std::string &path) {
  return fs::is_symlink(path);
}

std::string PathUtils::changeExtension(const std::string &path,
                                       const std::string &newExt) {
  fs::path p(path);
  p.replace_extension(newExt);
  return p.string();
}

std::string PathUtils::addSuffix(const std::string &path,
                                 const std::string &suffix) {
  fs::path p(path);
  std::string name = p.stem().string() + suffix + p.extension().string();
  return (p.parent_path() / name).string();
}

std::vector<std::string> PathUtils::components(const std::string &path) {
  std::vector<std::string> result;
  for (const auto &part : fs::path(path)) {
    std::string s = part.string();
    if (!s.empty() && s != "/")
      result.push_back(s);
  }
  return result;
}

std::string PathUtils::rootPath(const std::string &path) {
  return fs::path(path).root_path().string();
}

std::string PathUtils::expandTilde(const std::string &path) {
  if (path.empty() || path[0] != '~')
    return path;
  const char *home = std::getenv("HOME");
  if (!home)
    return path;
  return std::string(home) + path.substr(1);
}

std::string PathUtils::homeDirectory() {
  const char *home = std::getenv("HOME");
  return home ? home : "";
}

std::string PathUtils::currentDirectory() {
  return fs::current_path().string();
}

bool PathUtils::createDirectories(const std::string &path) {
  return fs::create_directories(path);
}

std::string PathUtils::uniquePath(const std::string &path) {
  if (!fs::exists(path))
    return path;

  fs::path p(path);
  std::string stem = p.stem().string();
  std::string ext = p.extension().string();
  fs::path parent = p.parent_path();

  int counter = 1;
  std::string newPath;
  do {
    newPath =
        (parent / (stem + "_" + std::to_string(counter++) + ext)).string();
  } while (fs::exists(newPath));

  return newPath;
}

} // namespace file
} // namespace darwincore
