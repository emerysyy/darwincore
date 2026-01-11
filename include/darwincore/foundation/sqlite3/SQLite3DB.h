//
//  SQLite3DB.h
//  SQLite
//
//  Created by Darwin Core on 2024/3/21.
//

#ifndef SQLITE3DB_H
#define SQLITE3DB_H


#include <sqlite3.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace darwincore {
    namespace sqlite {
        // ========================================
        // SQLite3 数据库类
        // ========================================
        // 功能：
        // 1. 封装 SQLite3 数据库操作
        // 2. 支持执行 SQL 语句和查询操作
        // 3. 提供简洁的查询结果访问接口
        // ========================================
        class SQLite3DB {
        public:
            // ========================================
            // 查询记录类
            // ========================================
            // 封装单条查询记录，支持按列名或索引访问
            // ========================================
            class Record {
                friend class SQLite3DB;
            private:
                Record();  // 私有构造函数，只能由 SQLite3DB 创建
            public:
                ~Record();

            public:
                // 获取列数
                // 返回：记录中的列数量
                int size();

                // 按索引获取列值
                // index: 列索引（从 0 开始）
                // value: 输出参数，存储列值
                void value(int index, std::string& value);

                // 按列名获取列值
                // name: 列名
                // value: 输出参数，存储列值
                void value(const std::string& name, std::string& value);

                // 按索引获取列名
                // index: 列索引（从 0 开始）
                // value: 输出参数，存储列名
                void name(int index, std::string& value);

            private:
                int _size;                                      // 列数
                std::map<std::string, std::string> _name_value_map;  // 列名到值的映射
                std::map<int, std::string> _index_name_map;     // 索引到列名的映射
            };

        public:
            // 构造函数
            // path: 数据库文件路径（":memory:" 表示内存数据库）
            explicit SQLite3DB(const std::string& path);
            ~SQLite3DB();

        public:
            // 打开数据库
            // 返回：true 表示成功，false 表示失败
            bool open();

            // 关闭数据库
            void close();

            // 执行 SQL 语句（不返回结果，如 INSERT、UPDATE、DELETE）
            // sql: SQL 语句字符串
            // 返回：true 表示执行成功，false 表示失败
            bool execute(const std::string& sql);

            // 查询 SQL 语句（返回结果，如 SELECT）
            // sql: SQL 查询语句字符串
            // result: 输出参数，存储查询结果（每条记录为一个 Record 对象）
            // 返回：true 表示查询成功，false 表示失败
            bool query(const std::string& sql, std::vector<Record>& result);


        private:
            /***
             @brief sqlite3_exec 方法第三个参数回调方法
             @param ctx sqlite3_exec 方法第四个参数 Context（用于传递回调数据）
             @param colNums 表示该条记录有多少列
             @param valueArr 返回的数据都蕴含在该变量中，它也是一个字符串数组。valueArr[n]即为第n列的数据。
             @param nameArr 存放列的名称，nameArr[n]就是第n列的名称。
             @return 返回 0 表示成功，非 0 表示失败
             */
            static int exe_callback(void *ctx, int colNums, char **valueArr, char **nameArr);


        private:
            sqlite3 *_handle;     // SQLite3 数据库句柄
            std::string _path;    // 数据库文件路径
        };
    };
};


#endif /* SQLITE3DB_H */
