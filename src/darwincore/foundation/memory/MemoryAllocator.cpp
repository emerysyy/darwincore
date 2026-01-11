//
// Created by 李培 on 2026/1/7.
//

#include <darwincore/foundation/memory/MemoryAllocator.h>
#include <cstddef>

namespace darwincore {
namespace memory {

Block *MemoryPool::free_lists[MemoryPool::MAX_BLOCK / MemoryPool::ALIGN] = {
    nullptr};

void *MemoryPool::allocate(std::size_t size) {
  if (size > MemoryPool::MAX_BLOCK)
    return ::operator new(size);
  std::size_t index = (size + MemoryPool::ALIGN - 1) / MemoryPool::ALIGN - 1;
  if (!free_lists[index])
    refill(index);
  Block *block = free_lists[index];
  free_lists[index] = block->next;
  return block;
}

void MemoryPool::deallocate(void *p, std::size_t size) {
  if (size > MemoryPool::MAX_BLOCK) {
    ::operator delete(p);
    return;
  }
  std::size_t index = (size + MemoryPool::ALIGN - 1) / MemoryPool::ALIGN - 1;
  Block *block = static_cast<Block *>(p);
  block->next = free_lists[index];
  free_lists[index] = block;
}

void MemoryPool::refill(std::size_t index) {
  std::size_t block_size = (index + 1) * MemoryPool::ALIGN;
  char *chunk = static_cast<char *>(::operator new(block_size * 10));
  for (std::size_t i = 0; i < 10; ++i) {
    Block *block = reinterpret_cast<Block *>(chunk + i * block_size);
    block->next = free_lists[index];
    free_lists[index] = block;
  }
}

} // namespace memory
} // namespace darwincore