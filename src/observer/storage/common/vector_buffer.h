#pragma once

#include "storage/common/arena_allocator.h"
#include "common/type/string_t.h"
#include "common/log/log.h"

/**
 * @file vector_buffer.h
 * @brief 为列式执行中的变长字符串提供集中式内存池。
 */

/**
 * @brief 使用 `Arena` 托管长字符串载荷的轻量缓冲区。
 * @details 短字符串依旧走 `string_t` 内联存储；只有超出内联阈值的数据才会落到 `Arena`。
 * 这样既避免了逐条 `malloc/free`，又能保证返回的 `string_t` 在 `VectorBuffer` 生命周期内稳定有效。
 */
class VectorBuffer
{
public:
  VectorBuffer() = default;

  /**
   * @brief 拷贝一段字符数据并返回对应的 `string_t`。
   * @details 当长度不超过内联阈值时不分配堆内存；否则会从 `Arena` 中切一段稳定空间保存内容。
   */
  string_t add_string(const char *data, int len)
  {
    if (len <= string_t::INLINE_LENGTH) {
      return string_t(data, len);
    }
    auto insert_string = empty_string(len);
    auto insert_pos    = insert_string.get_data_writeable();
    memcpy(insert_pos, data, len);
    return insert_string;
  }

  /// @brief 复用 `string_t` 的数据内容重新放入当前缓冲区。
  string_t add_string(string_t data) { return add_string(data.data(), data.size()); }

  /**
   * @brief 仅申请一段足够大的字符串空间，不立即写入内容。
   * @details 调用方可通过返回对象的可写指针填充数据，但必须保证写入长度与申请长度一致。
   */
  string_t empty_string(int len)
  {
    ASSERT(len > string_t::INLINE_LENGTH, "len > string_t::INLINE_LENGTH");
    auto insert_pos = heap.Allocate(len);
    return string_t(insert_pos, len);
  }

private:
  /// 统一托管所有长字符串载荷，析构时一次性释放。
  Arena heap;
};
