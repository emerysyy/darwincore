//
// Search.h
// DarwinCore
//

#ifndef DARWINCORE_SEARCH_H
#define DARWINCORE_SEARCH_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace darwincore {
namespace algorithm {

/// 搜索算法集合
class Search {
public:
  /// 二分查找（返回索引，未找到返回 nullopt）
  template <typename T, typename Compare = std::less<T>>
  static std::optional<size_t> binarySearch(const std::vector<T> &arr,
                                            const T &target,
                                            Compare comp = Compare{}) {
    if (arr.empty())
      return std::nullopt;

    size_t left = 0, right = arr.size();
    while (left < right) {
      size_t mid = left + (right - left) / 2;
      if (comp(arr[mid], target))
        left = mid + 1;
      else if (comp(target, arr[mid]))
        right = mid;
      else
        return mid;
    }
    return std::nullopt;
  }

  /// 查找第一个大于等于 target 的位置（lower_bound）
  template <typename T, typename Compare = std::less<T>>
  static size_t lowerBound(const std::vector<T> &arr, const T &target,
                           Compare comp = Compare{}) {
    size_t left = 0, right = arr.size();
    while (left < right) {
      size_t mid = left + (right - left) / 2;
      if (comp(arr[mid], target))
        left = mid + 1;
      else
        right = mid;
    }
    return left;
  }

  /// 查找第一个大于 target 的位置（upper_bound）
  template <typename T, typename Compare = std::less<T>>
  static size_t upperBound(const std::vector<T> &arr, const T &target,
                           Compare comp = Compare{}) {
    size_t left = 0, right = arr.size();
    while (left < right) {
      size_t mid = left + (right - left) / 2;
      if (comp(target, arr[mid]))
        right = mid;
      else
        left = mid + 1;
    }
    return left;
  }

  /// 线性查找
  template <typename T, typename Pred>
  static std::optional<size_t> linearSearch(const std::vector<T> &arr,
                                            Pred pred) {
    for (size_t i = 0; i < arr.size(); ++i)
      if (pred(arr[i]))
        return i;
    return std::nullopt;
  }

  /// 查找所有匹配项
  template <typename T, typename Pred>
  static std::vector<size_t> findAll(const std::vector<T> &arr, Pred pred) {
    std::vector<size_t> result;
    for (size_t i = 0; i < arr.size(); ++i)
      if (pred(arr[i]))
        result.push_back(i);
    return result;
  }

  /// 查找最大值的索引
  template <typename T, typename Compare = std::less<T>>
  static std::optional<size_t> findMax(const std::vector<T> &arr,
                                       Compare comp = Compare{}) {
    if (arr.empty())
      return std::nullopt;
    return std::max_element(arr.begin(), arr.end(), comp) - arr.begin();
  }

  /// 查找最小值的索引
  template <typename T, typename Compare = std::less<T>>
  static std::optional<size_t> findMin(const std::vector<T> &arr,
                                       Compare comp = Compare{}) {
    if (arr.empty())
      return std::nullopt;
    return std::min_element(arr.begin(), arr.end(), comp) - arr.begin();
  }

  /// 插值查找（适用于均匀分布的数值）
  template <typename T>
  static std::optional<size_t> interpolationSearch(const std::vector<T> &arr,
                                                   const T &target) {
    if (arr.empty())
      return std::nullopt;

    size_t left = 0, right = arr.size() - 1;
    while (left <= right && target >= arr[left] && target <= arr[right]) {
      if (left == right) {
        if (arr[left] == target)
          return left;
        return std::nullopt;
      }

      size_t pos = left + ((double)(right - left) / (arr[right] - arr[left])) *
                              (target - arr[left]);

      if (arr[pos] == target)
        return pos;
      if (arr[pos] < target)
        left = pos + 1;
      else
        right = pos - 1;
    }
    return std::nullopt;
  }
};

} // namespace algorithm
} // namespace darwincore

#endif // DARWINCORE_SEARCH_H
