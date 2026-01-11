//
// Sort.h
// DarwinCore
//

#ifndef DARWINCORE_SORT_H
#define DARWINCORE_SORT_H

#include <algorithm>
#include <functional>
#include <vector>

namespace darwincore {
namespace algorithm {

/// 排序算法集合
class Sort {
public:
  /// 快速排序
  template <typename T, typename Compare = std::less<T>>
  static void quickSort(std::vector<T> &arr, Compare comp = Compare{}) {
    if (arr.size() <= 1)
      return;
    quickSortImpl(arr, 0, arr.size() - 1, comp);
  }

  /// 归并排序
  template <typename T, typename Compare = std::less<T>>
  static void mergeSort(std::vector<T> &arr, Compare comp = Compare{}) {
    if (arr.size() <= 1)
      return;
    std::vector<T> temp(arr.size());
    mergeSortImpl(arr, temp, 0, arr.size() - 1, comp);
  }

  /// 堆排序
  template <typename T, typename Compare = std::less<T>>
  static void heapSort(std::vector<T> &arr, Compare comp = Compare{}) {
    if (arr.size() <= 1)
      return;

    // 建堆
    for (int i = arr.size() / 2 - 1; i >= 0; --i)
      heapify(arr, arr.size(), i, comp);

    // 逐个取出堆顶
    for (size_t i = arr.size() - 1; i > 0; --i) {
      std::swap(arr[0], arr[i]);
      heapify(arr, i, 0, comp);
    }
  }

  /// 插入排序（小数组优化）
  template <typename T, typename Compare = std::less<T>>
  static void insertionSort(std::vector<T> &arr, Compare comp = Compare{}) {
    for (size_t i = 1; i < arr.size(); ++i) {
      T key = std::move(arr[i]);
      size_t j = i;
      while (j > 0 && comp(key, arr[j - 1])) {
        arr[j] = std::move(arr[j - 1]);
        --j;
      }
      arr[j] = std::move(key);
    }
  }

  /// 检查是否已排序
  template <typename T, typename Compare = std::less<T>>
  static bool isSorted(const std::vector<T> &arr, Compare comp = Compare{}) {
    return std::is_sorted(arr.begin(), arr.end(), comp);
  }

  /// 部分排序（只排序前 k 个元素）
  template <typename T, typename Compare = std::less<T>>
  static void partialSort(std::vector<T> &arr, size_t k,
                          Compare comp = Compare{}) {
    if (k >= arr.size()) {
      std::sort(arr.begin(), arr.end(), comp);
    } else {
      std::partial_sort(arr.begin(), arr.begin() + k, arr.end(), comp);
    }
  }

  /// 第 k 小的元素（快速选择）
  template <typename T, typename Compare = std::less<T>>
  static T &nthElement(std::vector<T> &arr, size_t k,
                       Compare comp = Compare{}) {
    std::nth_element(arr.begin(), arr.begin() + k, arr.end(), comp);
    return arr[k];
  }

private:
  template <typename T, typename Compare>
  static void quickSortImpl(std::vector<T> &arr, size_t left, size_t right,
                            Compare &comp) {
    if (left >= right)
      return;

    size_t pivot = partition(arr, left, right, comp);
    if (pivot > 0)
      quickSortImpl(arr, left, pivot - 1, comp);
    quickSortImpl(arr, pivot + 1, right, comp);
  }

  template <typename T, typename Compare>
  static size_t partition(std::vector<T> &arr, size_t left, size_t right,
                          Compare &comp) {
    T pivot = arr[right];
    size_t i = left;
    for (size_t j = left; j < right; ++j) {
      if (comp(arr[j], pivot)) {
        std::swap(arr[i], arr[j]);
        ++i;
      }
    }
    std::swap(arr[i], arr[right]);
    return i;
  }

  template <typename T, typename Compare>
  static void mergeSortImpl(std::vector<T> &arr, std::vector<T> &temp,
                            size_t left, size_t right, Compare &comp) {
    if (left >= right)
      return;

    size_t mid = left + (right - left) / 2;
    mergeSortImpl(arr, temp, left, mid, comp);
    mergeSortImpl(arr, temp, mid + 1, right, comp);
    merge(arr, temp, left, mid, right, comp);
  }

  template <typename T, typename Compare>
  static void merge(std::vector<T> &arr, std::vector<T> &temp, size_t left,
                    size_t mid, size_t right, Compare &comp) {
    for (size_t i = left; i <= right; ++i)
      temp[i] = arr[i];

    size_t i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
      if (comp(temp[i], temp[j]))
        arr[k++] = temp[i++];
      else
        arr[k++] = temp[j++];
    }
    while (i <= mid)
      arr[k++] = temp[i++];
  }

  template <typename T, typename Compare>
  static void heapify(std::vector<T> &arr, size_t n, size_t i, Compare &comp) {
    size_t largest = i;
    size_t left = 2 * i + 1;
    size_t right = 2 * i + 2;

    if (left < n && comp(arr[largest], arr[left]))
      largest = left;
    if (right < n && comp(arr[largest], arr[right]))
      largest = right;

    if (largest != i) {
      std::swap(arr[i], arr[largest]);
      heapify(arr, n, largest, comp);
    }
  }
};

} // namespace algorithm
} // namespace darwincore

#endif // DARWINCORE_SORT_H
