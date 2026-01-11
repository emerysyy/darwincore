//
//  SQLite3DB.cpp
//  SQLite
//
//  Created by Darwin Core on 2024/3/21.
//

#include <darwincore/foundation/sqlite3/SQLite3DB.h>

using namespace darwincore::sqlite;

SQLite3DB::Record::Record() : _size(0) {
    
}

SQLite3DB::Record::~Record() {
    
}

int SQLite3DB::Record::size() {
    return _size;
}

void SQLite3DB::Record::value(int index, std::string &value) {
    std::string name;
    this->name(index, name);
    if (!name.empty()) {
        this->value(name, value);
    }
}

void SQLite3DB::Record::value(const std::string &name, std::string &value) {
    if (name.empty()) {
        return;
    }
    auto iter = _name_value_map.find(name);
    if (iter != _name_value_map.end()) {
        value = iter->second;
    }
}

void SQLite3DB::Record::name(int index, std::string &value) {
    if (index < 0 || index >= _size) {
        return;
    }
    
    auto iter = _index_name_map.find(index);
    if (iter != _index_name_map.end()) {
        value = iter->second;
    }
}

SQLite3DB::SQLite3DB(const std::string& path) : _path(path), _handle(nullptr) {
    
}

SQLite3DB::~SQLite3DB() {
    close();
}

bool SQLite3DB::open() {
    if (_path.empty()) {
        return false;
    }
    
    int ret = sqlite3_open(_path.c_str(), &_handle);
    if (ret != SQLITE_OK || !_handle) {
        return false;
    }
    return true;
}

void SQLite3DB::close() {
    if (_handle) {
        sqlite3_close(_handle);
        _handle = nullptr;
    }
}

bool SQLite3DB::execute(const std::string &sql) {
    if (_handle == nullptr) {
        printf("%s database is not opened\n", __FUNCTION__);
        return false;
    }
    
    if (sql.empty()) {
        printf("%s sql is empty\n", __FUNCTION__);
        return false;
    }
    
    char *errMsg = nullptr;
    int ret = sqlite3_exec(_handle, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (ret != SQLITE_OK) {
        printf("%s exec failed:%s\n", __FUNCTION__, (errMsg == nullptr ? "Unknown Error" : errMsg));
    }

    if (errMsg) {
        sqlite3_free(errMsg);
        errMsg = nullptr;
    }

    return ret == SQLITE_OK;
}

bool SQLite3DB::query(const std::string &sql, std::vector<Record> &result) {
    if (_handle == nullptr) {
        printf("%s database is not opened\n", __FUNCTION__);
        return false;
    }
    
    if (sql.empty()) {
        printf("%s sql is empty\n", __FUNCTION__);
        return false;
    }
    
    char *errMsg = nullptr;
    int ret = sqlite3_exec(_handle, sql.c_str(), &SQLite3DB::exe_callback, &result, &errMsg);
    
    if (ret != SQLITE_OK) {
        printf("%s exec failed:%s\n", __FUNCTION__, (errMsg == nullptr ? "Unknown Error" : errMsg));
    }

    if (errMsg) {
        sqlite3_free(errMsg);
        errMsg = nullptr;
    }

    return ret == SQLITE_OK;
}

int SQLite3DB::exe_callback(void *ctx, int colNums, char **valueArr, char **nameArr) {
    if (ctx == nullptr) {
        return SQLITE_OK;
    }
    
    std::vector<Record> *result = (std::vector<Record> *)ctx;
    
    Record record;
    record._size = colNums;
    
    for (int i = 0; i < colNums; i++) {
        std::string name = nameArr[i];
        std::string value = valueArr[i];
        record._index_name_map.emplace(i, name);
        record._name_value_map.emplace(name, value);
    }
    
    result->push_back(record);
    
    return SQLITE_OK;
}
