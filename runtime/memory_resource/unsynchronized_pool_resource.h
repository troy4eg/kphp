#pragma once

#include "runtime/memory_resource/details/memory_chunk_list.h"
#include "runtime/memory_resource/details/memory_chunk_tree.h"
#include "runtime/memory_resource/details/universal_reallocate.h"
#include "runtime/memory_resource/monotonic_buffer_resource.h"
#include "runtime/memory_resource/resource_allocator.h"
#include "runtime/php_assert.h"

namespace memory_resource {

class unsynchronized_pool_resource : private monotonic_buffer_resource {
public:
  using monotonic_buffer_resource::try_expand;
  using monotonic_buffer_resource::get_memory_stats;
  using monotonic_buffer_resource::memory_begin;

  void init(void *buffer, size_type buffer_size) noexcept;

  void *allocate(size_type size) noexcept {
    void *mem = nullptr;
    const auto aligned_size = details::align_for_chunk(size);
    if (aligned_size < MAX_CHUNK_BLOCK_SIZE_) {
      mem = try_allocate_small_piece(aligned_size);
      if (!mem) {
        mem = allocate_small_piece_from_fallback_resource(aligned_size);
      }
    } else {
      mem = allocate_huge_piece(aligned_size, true);
      if (!mem) {
        mem = perform_defragmentation_and_allocate_huge_piece(aligned_size);
      }
    }

    register_allocation(mem, aligned_size);
    return mem;
  }

  void *allocate0(size_type size) noexcept {
    auto mem = allocate(size);
    if (likely(mem != nullptr)) {
      memset(mem, 0x00, size);
    }
    return mem;
  }

  void *reallocate(void *mem, size_type new_size, size_type old_size) noexcept {
    const auto aligned_old_size = details::align_for_chunk(old_size);
    const auto aligned_new_size = details::align_for_chunk(new_size);
    return details::universal_reallocate(*this, mem, aligned_new_size, aligned_old_size);
  }

  void deallocate(void *mem, size_type size) noexcept {
    memory_debug("deallocate %d at %p\n", size, mem);
    const auto aligned_size = details::align_for_chunk(size);
    put_memory_back(mem, aligned_size);
    register_deallocation(aligned_size);
  }

  void perform_defragmentation() noexcept;

  bool is_enough_memory_for(size_type size) const noexcept {
    const auto aligned_size = details::align_for_chunk(size);
    // не смотрим в free_chunks_, так как реальный размер может оказаться меньше
    return memory_end_ - memory_current_ >= aligned_size || huge_pieces_.has_memory_for(aligned_size);
  }

private:
  void *try_allocate_small_piece(size_type aligned_size) noexcept {
    const auto chunk_id = details::get_chunk_id(aligned_size);
    auto *mem = free_chunks_[chunk_id].get_mem();
    if (mem) {
      --stats_.small_memory_pieces;
      memory_debug("allocate %d, chunk found, allocated address %p\n", aligned_size, mem);
      return mem;
    }
    mem = get_from_pool(aligned_size, true);
    memory_debug("allocate %d, chunk not found, allocated address from pool %p\n", aligned_size, mem);
    return mem;
  }

  void *allocate_huge_piece(size_type aligned_size, bool safe) noexcept {
    void *mem = nullptr;
    if (details::memory_chunk_tree::tree_node *piece = huge_pieces_.extract(aligned_size)) {
      --stats_.huge_memory_pieces;
      const size_type real_size = details::memory_chunk_tree::get_chunk_size(piece);
      mem = piece;
      if (const size_type left = real_size - aligned_size) {
        put_memory_back(static_cast<char *>(mem) + aligned_size, left);
      }
      memory_debug("allocate %d, huge chunk (%ud) found, allocated address %p\n", aligned_size, real_size, mem);
      return piece;
    }
    mem = get_from_pool(aligned_size, safe);
    memory_debug("allocate %d, huge chunk not found, allocated address from pool %p\n", aligned_size, mem);
    return mem;
  }

  void *allocate_small_piece_from_fallback_resource(size_type aligned_size) noexcept;
  void *perform_defragmentation_and_allocate_huge_piece(size_type aligned_size) noexcept;

  void put_memory_back(void *mem, size_type size) noexcept {
    if (!monotonic_buffer_resource::put_memory_back(mem, size)) {
      if (size < MAX_CHUNK_BLOCK_SIZE_) {
        size_type chunk_id = details::get_chunk_id(size);
        free_chunks_[chunk_id].put_mem(mem);
        ++stats_.small_memory_pieces;
      } else {
        huge_pieces_.insert(mem, size);
        ++stats_.huge_memory_pieces;
      }
    }
  }

  details::memory_chunk_tree huge_pieces_;
  monotonic_buffer_resource fallback_resource_;

  static constexpr size_type MAX_CHUNK_BLOCK_SIZE_{16u * 1024u};
  std::array<details::memory_chunk_list, details::get_chunk_id(MAX_CHUNK_BLOCK_SIZE_)> free_chunks_;
};

} // namespace memory_resource
