//
// Created by Darwin Core on 2023/1/28.
//

#include <darwincore/foundation/file/FilePath.h>

#include <darwincore/foundation/string/StringUtils.h>
#include <utility>

using namespace darwincore::file;
using namespace darwincore::string;

static std::string FilePathSeparator = "/";

FilePath::FilePath(std::vector<std::string> components) {
  mPathComponents = std::move(components);
}

FilePath::FilePath(const std::string &path) { extractPathComponents(path); }

std::string FilePath::toString() {

  if (mPathComponents.empty()) {
    return "";
  }

  return assemblyPath(mPathComponents);
}

std::string FilePath::extensionName() {
  std::string fname = displayName();

  auto iter = fname.find_last_of(".");

  if (iter == std::string::npos || iter == (fname.size() - 1)) {
    return "";
  }

  auto begin = iter + 1;
  return fname.substr(begin, fname.size() - begin);
}

std::string FilePath::name() {
  std::string fname = displayName();

  auto iter = fname.find_last_of(".");

  if (iter == std::string::npos || iter == (fname.size() - 1)) {
    return fname;
  }

  auto begin = fname.begin();
  return fname.substr(0, iter);
}

std::string FilePath::displayName() {
  if (mPathComponents.empty()) {
    return "";
  }

  auto &last = mPathComponents.back();

  return last;
}

std::string FilePath::parentDir() {
  if (mPathComponents.empty()) {
    return "";
  }

  if (mPathComponents.size() == 1) {
    return FilePathSeparator;
  }

  std::vector<std::string> tmp = mPathComponents;
  tmp.pop_back();
  return assemblyPath(tmp);
}

std::string FilePath::append(const std::string &name) {

  std::string tmp = toString() + FilePathSeparator + name;
  return tmp;
}

std::string FilePath::assemblyPath(const std::vector<std::string> &components) {
  std::string path("");
  for (auto &name : components) {
    path += FilePathSeparator + name;
  }
  return path;
}

void FilePath::extractPathComponents(const std::string &path) {
  std::vector<std::string> names = StringUtils::split(path, FilePathSeparator);
  std::vector<std::string> stack;

  for (auto &name : names) {
    if (name.empty()) {
      continue;
    }
    if (name == ".." && !stack.empty()) {
      stack.pop_back();
      continue;
    }

    if (name != ".") {
      stack.push_back(std::move(name));
    }
  }
  mPathComponents.swap(stack);
}

FilePath FilePath::parent() {
  if (mPathComponents.empty()) {
    return FilePath("");
  }

  if (mPathComponents.size() == 1) {
    return FilePath(FilePathSeparator);
  }

  std::vector<std::string> tmp = mPathComponents;
  tmp.pop_back();
  return FilePath(tmp);
}

FilePath FilePath::appendNode(const std::string &name) {
  std::vector<std::string> tmp = mPathComponents;
  if (!name.empty()) {
    tmp.push_back(name);
  }
  return FilePath(tmp);
}
