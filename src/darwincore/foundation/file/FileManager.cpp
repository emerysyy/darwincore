//
// Created by Darwin Core on 2023/1/28.
//

#include <darwincore/foundation/file/FileManager.h>
#include <darwincore/foundation/file/FilePath.h>

#include <fstream>

#include <dirent.h> // opendir, readdir
#include <sys/stat.h>
#include <unistd.h> // ftruncate

using namespace darwincore::file;

std::vector<std::string> FileManager::subpathsAtPath(const std::string &path) {
  std::vector<std::string> items;
  do {

    bool isDir = false;
    if (path.empty() || !isItemExistsAtPath(path, isDir) || !isDir) {
      break;
    }

    DIR *dirp = opendir(path.c_str());
    if (dirp == NULL) {
      break;
    }
    FilePath fp(path);

    struct dirent *ptr;
    while ((ptr = readdir(dirp)) != NULL) {

      std::string fname(ptr->d_name);
      if (fname == "." || fname == ".." || fname == ".DS_Store") {
        continue;
      }

      std::string fpath = fp.append(fname);
      items.push_back(fpath);
    }

  } while (false);

  return items;
}

std::string FileManager::displayNameAtPath(const std::string &path,
                                           bool hideExtension) {

  FilePath fp(path);
  std::string name = fp.displayName();

  if (hideExtension) {
    auto iter = name.find_last_of(".");
    if (iter != std::string::npos) {
      name = name.substr(0, iter);
    }
  }

  return name;
}

bool FileManager::createDirectoryAtPath(const std::string &path, int mode,
                                        bool createIntermediates) {
  FilePath fp(path);
  bool isDir = false;
  if (isItemExistsAtPath(path, isDir) && isDir) {
    return false;
  }

  std::string parentDir = fp.parentDir();
  bool isParentExists = isItemExistsAtPath(parentDir);

  if (isParentExists) {
    return mkdir(path.c_str(), mode) == 0;
  } else {
    if (!createIntermediates) {
      return false;
    } else {
      if (!createDirectoryAtPath(parentDir, createIntermediates)) {
        return false;
      }
    }
    return mkdir(path.c_str(), mode) == 0;
  }

  return false;
}

bool FileManager::createDirectoryAtPath(const std::string &path,
                                        bool createIntermediates) {

  return createDirectoryAtPath(path, 0777, createIntermediates);
}

bool FileManager::isItemExistsAtPath(const std::string &path) {

  bool isDir = false;
  return isItemExistsAtPath(path, isDir);
}

bool FileManager::isItemExistsAtPath(const std::string &path,
                                     bool &isDirectory) {

  struct stat buffer;
  bool ret = stat(path.c_str(), &buffer) == 0;

  if (ret) {
    isDirectory = buffer.st_mode & S_IFDIR;
  }

  return ret;
}

bool FileManager::removeItemAtPath(const std::string &path) {

  bool isDir = false;

  if (!isItemExistsAtPath(path, isDir)) {
    return false;
  }

  if (isDir) {
    std::vector<std::string> subpaths = subpathsAtPath(path);
    for (std::string &sp : subpaths) {
      if (!removeItemAtPath(sp)) {
        return false;
      }
    }
  }
  return remove(path.c_str()) == 0;
}

bool FileManager::moveItem(const std::string &fromPath,
                           const std::string &toPath, bool cover) {

  if (isItemExistsAtPath(toPath)) {
    if (!cover) {
      return false;
    }
    if (!removeItemAtPath(toPath)) {
      return false;
    }
  }

  return renameItem(fromPath, toPath);
}

bool FileManager::copyItem(const std::string &fromPath,
                           const std::string &toPath, bool cover) {

  if (fromPath.empty() || toPath.empty()) {
    return false;
  }

  bool isDir = false;
  if (!isItemExistsAtPath(fromPath, isDir)) {
    return false;
  }

  bool destIsDir = false;
  bool destExists = isItemExistsAtPath(toPath, destIsDir);
  if (destExists) {
    if (cover) {
      if (!removeItemAtPath(toPath)) {
        return false;
      }
    } else {
      return false;
    }
  }

  if (isDir) {
    createDirectoryAtPath(toPath);

    std::vector<std::string> subArr = subpathsAtPath(fromPath);
    for (std::string &f : subArr) {
      FilePath fp(f);
      std::string subTo = FilePath(toPath).append(fp.displayName());
      if (!copyItem(f, subTo, cover)) {
        return false;
      }
    }
  } else {

    FilePath fp(toPath);
    std::string parentDir = fp.parentDir();
    if (!isItemExistsAtPath(parentDir)) {
      if (!createDirectoryAtPath(parentDir)) {
        return false;
      }
    }

    std::ifstream ifs(fromPath.c_str(), std::ios::binary);
    std::ofstream ofs(toPath.c_str(), std::ios::binary);

    ofs << ifs.rdbuf();

    ifs.close();
    ofs.close();
  }

  return true;
}

bool FileManager::renameItem(const std::string &fromPath,
                             const std::string &toPath) {

  if (fromPath.empty() || toPath.empty()) {
    return false;
  }

  if (!isItemExistsAtPath(fromPath)) {
    return false;
  }

  if (isItemExistsAtPath(toPath)) {
    return false;
  }

  return rename(fromPath.c_str(), toPath.c_str()) == 0;
}

bool FileManager::cleanFile(const std::string &path) {

  if (path.empty()) {
    return false;
  }

  bool isDir = false;
  if (!isItemExistsAtPath(path, isDir) || isDir) {
    return false;
  }

  FILE *f = fopen(path.c_str(), "w+");
  if (f == NULL) {
    return false;
  }

  bool ret = false;
  do {
    int fd = fileno(f);
    // 将参数fd指定的文件大小改为参数length指定的大小。参数fd为已打开的文件描述词，而且必须是以写入模式打开的文件。
    // 如果原来的文件件大小比参数length大，则超过的部分会被删去
    if (ftruncate(fd, 0) != 0) {
      break;
    }

    lseek(fd, 0, SEEK_SET);

    ret = true;

  } while (false);

  fclose(f);

  return ret;
}

bool FileManager::copyItem(const std::string &fromPath,
                           const std::string toPath, double percentage) {

  FILE *source = fopen(fromPath.c_str(), "rb");
  if (!source) {
    return false;
  }

  fseek(source, 0, SEEK_END);
  long size = ftell(source);
  fseek(source, 0, SEEK_SET);

  FILE *dest = fopen(toPath.c_str(), "wb");
  if (!dest) {
    return false;
  }

  char *buf[1024] = {0};
  size_t n = 0;
  size_t copied = 0;
  while ((n = fread(buf, 1, sizeof(buf), source)) > 0) {
    if (fwrite(buf, 1, n, dest) != n) {
      printf("write error");
    }
    copied += n;
    if (copied >= (long)(size * percentage)) {
      break;
    }
  }

  fclose(dest);
  fclose(source);

  return true;
}

bool FileManager::getCurrentWorkSpace(std::string &workSpace) {
  char currentPath[FILENAME_MAX];
  if (getcwd(currentPath, sizeof(currentPath)) != nullptr) {
    workSpace = currentPath;
    return true;
  }
  return false;
}

bool FileManager::changeCurrentWorkSpace(std::string &workSpace) {
  if (workSpace.empty()) {
    return false;
  }
  if (chdir(workSpace.c_str()) == 0) {
    return true;
  }
  return false;
}
