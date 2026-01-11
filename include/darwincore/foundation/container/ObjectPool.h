//
// ObjectPool.h
// DarwinCore
//
// Created by DarwinCore on 2026/1/7.
// Copyright © 2026 DarwinCore. All rights reserved.
//

#ifndef DARWINCORE_OBJECT_POOL_H
#define DARWINCORE_OBJECT_POOL_H

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <stack>
#include <stdexcept>
#include <vector>

namespace darwincore {
namespace container {

/**
 * @brief 对象池（Object Pool）
 *
 * ---------------------------------------------------------------------------
 * 设计目的
 * ---------------------------------------------------------------------------
 * ObjectPool 用于集中管理「构造/析构成本较高」且「生命周期短、可复用」
 * 的对象，通过对象复用来：
 *
 * - 避免频繁 new/delete 带来的性能损耗与堆内存碎片
 * - 控制资源并发上限（maxSize）
 * - 统一对象初始化与回收逻辑（Create / Reset）
 * - 通过 RAII 自动归还，避免资源泄漏
 *
 * 本实现提供：
 * - 非线程安全版本（ObjectPool）
 * - 基于 mutex 的线程安全包装（ThreadSafeObjectPool）
 *
 * ---------------------------------------------------------------------------
 * 适合使用 ObjectPool 的场景（When to Use）
 * ---------------------------------------------------------------------------
 *
 * 【一】对象构造 / 析构成本高
 *  - 构造时需要：
 *    - 分配大块内存
 *    - 初始化复杂结构
 *    - 打开系统资源（fd、Mach port、XPC connection）
 *
 * 【二】对象生命周期短、使用频繁
 *  - 在热点路径反复创建和销毁
 *  - 使用完成后即可回收复用
 *
 * 【三】对象状态可以被可靠地 reset
 *  - 可以通过 resetFunc 恢复到“刚创建时的等价状态”
 *  - 不携带跨生命周期的隐式状态
 *
 * 【四】需要限制并发数量或资源总量
 *  - maxSize != 0 时，对象池同时承担“资源配额管理器”角色
 *
 * ---------------------------------------------------------------------------
 * macOS 数据安全终端（DLP / Zero Trust / EDR）中的典型使用场景
 * ---------------------------------------------------------------------------
 *
 * 【1】EndpointSecurity 事件处理上下文
 *  - 每条 ES 事件（exec, open, write, rename）对应一个解析上下文对象
 *  - 包含：
 *      - 进程快照
 *      - 路径解析缓存
 *      - 策略匹配中间结果
 *  - 事件频繁、生命周期短，非常适合对象池复用
 *
 * 【2】DLP 内容检测中间对象
 *  - 文件扫描上下文（FileScanContext）
 *  - 文本/二进制内容缓冲区
 *  - 正则 / 特征匹配临时状态
 *
 * 【3】XPC / IPC 请求处理对象
 *  - 每个 XPC 请求对应一个 RequestContext
 *  - 含反序列化结果、权限校验信息、返回缓冲区
 *
 * 【4】加解密 / 安全计算上下文
 *  - 对称加密上下文（AES/GCM）
 *  - 哈希计算状态（SHA / HMAC）
 *  - 硬件/系统安全模块句柄的包装对象
 *
 * 【5】日志 / 审计事件对象
 *  - 高并发安全日志事件
 *  - 日志结构体较大但格式固定
 *
 * ---------------------------------------------------------------------------
 * 不适合使用 ObjectPool 的场景（When NOT to Use）
 * ---------------------------------------------------------------------------
 *
 * 【一】轻量、POD 或 trivially destructible 对象
 *  - 如：int、struct { int x; int y; }
 *  - 栈分配或直接 new/delete 成本更低
 *
 * 【二】对象状态复杂，难以彻底 reset
 *  - 内部持有跨生命周期的全局引用
 *  - reset 后仍可能残留隐式状态
 *
 *  ❗ 若 resetFunc 不能保证“逻辑等价于新建对象”，请勿池化
 *
 * 【三】对象生命周期不可控或需要长期持有
 *  - 被缓存到全局结构
 *  - 生命周期跨多个模块或线程
 *
 * 【四】极端高并发、低延迟场景
 *  - ThreadSafeObjectPool 使用 std::mutex
 *  - 不适合 lock-free 或纳秒级延迟要求
 *
 *  此类场景应考虑：
 *  - per-thread pool
 *  - slab allocator
 *  - 专用内存池 / arena
 *
 * 【五】对象池生命周期短于对象使用周期
 *  - ObjectPool 已析构，但 PooledObject 仍存在
 *  - 属于严重逻辑错误（Undefined Behavior）
 *
 * ---------------------------------------------------------------------------
 * 使用建议（Best Practices）
 * ---------------------------------------------------------------------------
 *
 * - 强烈建议提供 resetFunc，确保对象归还池中前状态干净
 * - 使用 PooledObject（RAII）而非裸指针，避免忘记归还
 * - 若需要对象“逃离池”，可显式调用 PooledObject::release()
 * - 在多线程环境下使用 ThreadSafeObjectPool
 *
 * ---------------------------------------------------------------------------
 * 总结
 * ---------------------------------------------------------------------------
 * ObjectPool 并不是通用替代 new/delete 的工具，而是：
 *
 *  - 面向高成本对象
 *  - 面向高频路径
 *  - 面向资源可控场景
 *
 * 在 macOS 数据安全终端中，它非常适合用于
 * “事件驱动 + 高并发 + 生命周期短”的核心中间对象管理。
 *
 * @tparam T 池化对象类型
 *
 * ---------------------------------------------------------------------------
 * 使用方式（How to Use）
 * ---------------------------------------------------------------------------
 *
 * 【一】最基本用法（单线程）
 *
 * 1. 定义对象类型
 *
 *   struct TaskContext {
 *       int id;
 *       std::string data;
 *       void reset() {
 *           id = 0;
 *           data.clear();
 *       }
 *   };
 *
 * 2. 创建对象池
 *
 *   ObjectPool<TaskContext> pool(
 *       8, // initialSize：预创建对象数量
 *       [] {
 *           return std::make_unique<TaskContext>();
 *       },
 *       [](TaskContext& ctx) {
 *           ctx.reset(); // resetFunc：归还前重置状态
 *       },
 *       32 // maxSize：最大对象数量（0 表示不限制）
 *   );
 *
 * 3. 获取并使用对象（RAII）
 *
 *   {
 *       auto ctx = pool.acquire();
 *       if (!ctx) {
 *           // 已达最大容量，获取失败
 *           return;
 *       }
 *       ctx->id = 1001;
 *       ctx->data = "example";
 *       // 离开作用域后，ctx 自动归还对象池
 *   }
 *
 * ---------------------------------------------------------------------------
 * 【二】多线程场景
 *
 * 使用 ThreadSafeObjectPool 进行线程安全访问。
 * 适用于：
 *  - 多个工作线程并发处理事件
 *  - 但对象获取/归还不是极端热点路径
 *
 *   ThreadSafeObjectPool<TaskContext> pool(
 *       16,
 *       [] { return std::make_unique<TaskContext>(); },
 *       [](TaskContext& ctx) { ctx.reset(); },
 *       64
 *   );
 *
 *   void worker() {
 *       auto ctx = pool.acquire();
 *       if (!ctx) return;
 *       // 使用 ctx
 *   }
 *
 * ---------------------------------------------------------------------------
 * 【三】仅复用已有对象，不允许新建
 *
 * tryAcquire() 只从池中取空闲对象，不会创建新对象。
 * 常用于：
 *  - 严格限制资源使用
 *  - 不允许动态扩容的安全路径
 *
 *   auto obj = pool.tryAcquire();
 *   if (!obj) {
 *       // 当前无可用资源，直接拒绝或降级
 *       return;
 *   }
 *
 * ---------------------------------------------------------------------------
 * 【四】对象“脱离对象池”（高级用法）
 *
 * 当对象需要：
 *  - 跨模块长期持有
 *  - 生命周期不再受池管理
 *
 * 可显式释放池绑定关系：
 *
 *   auto pooled = pool.acquire();
 *   std::unique_ptr<T> obj = pooled.release();
 *   // obj 不会再自动归还池中，需要自行管理生命周期
 *
 * ⚠ 注意：一旦 release()，该对象不再受 ObjectPool 管理
 *
 * ---------------------------------------------------------------------------
 * macOS 数据安全终端中的典型使用示例
 * ---------------------------------------------------------------------------
 *
 * 【1】EndpointSecurity 事件上下文池
 *
 *   struct ESEventContext {
 *       pid_t pid;
 *       std::string processPath;
 *       std::string targetPath;
 *
 *       void reset() {
 *           pid = 0;
 *           processPath.clear();
 *           targetPath.clear();
 *       }
 *   };
 *
 *   ThreadSafeObjectPool<ESEventContext> esContextPool(
 *       64,
 *       [] { return std::make_unique<ESEventContext>(); },
 *       [](ESEventContext& ctx) { ctx.reset(); },
 *       256
 *   );
 *
 *   void handle_es_event(...) {
 *       auto ctx = esContextPool.acquire();
 *       if (!ctx) {
 *           // 资源耗尽，拒绝或降级处理
 *           return;
 *       }
 *
 *       ctx->pid = event->process->pid;
 *       ctx->processPath = ...;
 *       ctx->targetPath = ...;
 *
 *       // 执行策略匹配、审计、上报
 *       // 离开作用域后自动归还
 *   }
 *
 * 【2】DLP 文件扫描上下文
 *
 *  - 扫描过程中使用的大缓冲区
 *  - 正则 / 特征匹配中间状态
 *
 *   struct FileScanContext {
 *       std::vector<uint8_t> buffer;
 *       void reset() {
 *           buffer.clear();
 *       }
 *   };
 *
 *   ObjectPool<FileScanContext> scanPool(
 *       8,
 *       [] {
 *           auto ctx = std::make_unique<FileScanContext>();
 *           ctx->buffer.reserve(1024 * 1024); // 预分配 1MB
 *           return ctx;
 *       },
 *       [](FileScanContext& ctx) { ctx.reset(); },
 *       32
 *   );
 *
 * ---------------------------------------------------------------------------
 * 使用注意事项
 * ---------------------------------------------------------------------------
 *
 * - acquire() 可能返回空对象（达到 maxSize）
 * - resetFunc 应保证对象状态干净、可复用
 * - ObjectPool 生命周期必须长于 PooledObject
 * - 不要将 PooledObject 跨线程随意移动
 *
 */

template <typename T> class ObjectPool {
public:
  using CreateFunc = std::function<std::unique_ptr<T>()>;
  using ResetFunc = std::function<void(T &)>;
  using DestroyFunc = std::function<void(T &)>;

  /**
   * @brief 池化对象包装器
   *
   * 使用 RAII 模式，析构时自动将对象归还池中。
   */
  class PooledObject {
  public:
    PooledObject(std::unique_ptr<T> obj, ObjectPool *pool)
        : object_(std::move(obj)), pool_(pool) {}

    ~PooledObject() {
      if (object_ && pool_) {
        pool_->release(std::move(object_));
      }
    }

    // 禁用拷贝
    PooledObject(const PooledObject &) = delete;
    PooledObject &operator=(const PooledObject &) = delete;

    // 支持移动
    PooledObject(PooledObject &&other) noexcept
        : object_(std::move(other.object_)), pool_(other.pool_) {
      other.pool_ = nullptr;
    }

    PooledObject &operator=(PooledObject &&other) noexcept {
      if (this != &other) {
        if (object_ && pool_) {
          pool_->release(std::move(object_));
        }
        object_ = std::move(other.object_);
        pool_ = other.pool_;
        other.pool_ = nullptr;
      }
      return *this;
    }

    T *get() const { return object_.get(); }
    T *operator->() const { return object_.get(); }
    T &operator*() const { return *object_; }
    explicit operator bool() const { return object_ != nullptr; }

    /**
     * @brief 释放对象所有权（不归还池）
     */
    std::unique_ptr<T> release() {
      pool_ = nullptr;
      return std::move(object_);
    }

  private:
    std::unique_ptr<T> object_;
    ObjectPool *pool_;
  };

  /**
   * @brief 构造函数
   *
   * @param initialSize 初始对象数量
   * @param createFunc 对象创建函数
   * @param resetFunc 对象重置函数（归还时调用）
   * @param maxSize 最大对象数量（0表示无限制）
   */
  ObjectPool(size_t initialSize, CreateFunc createFunc,
             ResetFunc resetFunc = nullptr, size_t maxSize = 0)
      : createFunc_(std::move(createFunc)), resetFunc_(std::move(resetFunc)),
        maxSize_(maxSize), createdCount_(0) {

    if (!createFunc_) {
      throw std::invalid_argument("ObjectPool requires a create function");
    }

    // 预创建对象
    for (size_t i = 0; i < initialSize; ++i) {
      pool_.push(createFunc_());
      ++createdCount_;
    }
  }

  ~ObjectPool() = default;

  // 禁用拷贝
  ObjectPool(const ObjectPool &) = delete;
  ObjectPool &operator=(const ObjectPool &) = delete;

  // 支持移动
  ObjectPool(ObjectPool &&) = default;
  ObjectPool &operator=(ObjectPool &&) = default;

  /**
   * @brief 从池中获取对象
   *
   * 如果池中有可用对象，直接返回。
   * 如果池为空且未达到最大限制，创建新对象。
   * 如果已达到最大限制，返回空指针。
   *
   * @return 池化对象包装器
   */
  PooledObject acquire() {
    if (!pool_.empty()) {
      auto obj = std::move(pool_.top());
      pool_.pop();
      return PooledObject(std::move(obj), this);
    }

    // 池为空，尝试创建新对象
    if (maxSize_ == 0 || createdCount_ < maxSize_) {
      auto obj = createFunc_();
      ++createdCount_;
      return PooledObject(std::move(obj), this);
    }

    // 已达最大限制
    return PooledObject(nullptr, nullptr);
  }

  /**
   * @brief 尝试获取对象（非阻塞）
   *
   * 仅当池中有可用对象时返回对象，否则返回空。
   * 不会创建新对象。
   */
  PooledObject tryAcquire() {
    if (pool_.empty()) {
      return PooledObject(nullptr, nullptr);
    }
    auto obj = std::move(pool_.top());
    pool_.pop();
    return PooledObject(std::move(obj), this);
  }

  /**
   * @brief 获取池中可用对象数量
   */
  [[nodiscard]] size_t available() const { return pool_.size(); }

  /**
   * @brief 获取已创建的对象总数
   */
  [[nodiscard]] size_t totalCreated() const { return createdCount_; }

  /**
   * @brief 获取当前借出的对象数量
   */
  [[nodiscard]] size_t borrowed() const { return createdCount_ - pool_.size(); }

  /**
   * @brief 清空池中的空闲对象
   */
  void clear() {
    while (!pool_.empty()) {
      pool_.pop();
      --createdCount_;
    }
  }

  /**
   * @brief 预热池
   *
   * 确保池中至少有指定数量的可用对象。
   */
  void warmUp(size_t count) {
    while (pool_.size() < count) {
      if (maxSize_ > 0 && createdCount_ >= maxSize_) {
        break;
      }
      pool_.push(createFunc_());
      ++createdCount_;
    }
  }

private:
  friend class PooledObject;

  /**
   * @brief 归还对象到池中
   */
  void release(std::unique_ptr<T> obj) {
    if (resetFunc_) {
      resetFunc_(*obj);
    }
    pool_.push(std::move(obj));
  }

  CreateFunc createFunc_;
  ResetFunc resetFunc_;
  size_t maxSize_;
  size_t createdCount_;
  std::stack<std::unique_ptr<T>> pool_;
};

/**
 * @brief 线程安全的对象池
 */
template <typename T> class ThreadSafeObjectPool {
public:
  using CreateFunc = typename ObjectPool<T>::CreateFunc;
  using ResetFunc = typename ObjectPool<T>::ResetFunc;
  using PooledObject = typename ObjectPool<T>::PooledObject;

  ThreadSafeObjectPool(size_t initialSize, CreateFunc createFunc,
                       ResetFunc resetFunc = nullptr, size_t maxSize = 0)
      : pool_(initialSize, std::move(createFunc), std::move(resetFunc),
              maxSize) {}

  PooledObject acquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.acquire();
  }

  PooledObject tryAcquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.tryAcquire();
  }

  size_t available() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.available();
  }

  size_t totalCreated() {
    std::lock_guard<std::mutex> lock(mutex_);
    return pool_.totalCreated();
  }

  void clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.clear();
  }

  void warmUp(size_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.warmUp(count);
  }

private:
  ObjectPool<T> pool_;
  std::mutex mutex_;
};

} // namespace container
} // namespace darwincore

#endif // DARWINCORE_OBJECT_POOL_H
