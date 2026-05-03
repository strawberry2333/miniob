/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/log/log.h"
#include "storage/common/column.h"

/**
 * @file column.cpp
 * @brief `Column` 的定长列缓冲区实现。
 */

Column::Column(const FieldMeta &meta, size_t size)
    : data_(nullptr),
      count_(0),
      capacity_(0),
      own_(true),
      attr_type_(meta.type()),
      attr_len_(meta.len()),
      column_type_(Type::NORMAL_COLUMN)
{
  // 普通列总是预先分配满容量空间，优先保证批处理写入时的顺序内存布局。
  data_     = new char[size * attr_len_];
  memset(data_, 0, size * attr_len_);
  capacity_ = size;
}

Column::Column(AttrType attr_type, int attr_len, size_t capacity)
{
  attr_type_   = attr_type;
  attr_len_    = attr_len;
  data_        = new char[capacity * attr_len_];
  memset(data_, 0, capacity * attr_len_);
  count_       = 0;
  capacity_    = capacity;
  own_         = true;
  column_type_ = Type::NORMAL_COLUMN;
}

void Column::init(const FieldMeta &meta, size_t size)
{
  reset();
  data_        = new char[size * meta.len()];
  memset(data_, 0, size * meta.len());
  count_       = 0;
  capacity_    = size;
  attr_type_   = meta.type();
  attr_len_    = meta.len();
  own_         = true;
  column_type_ = Type::NORMAL_COLUMN;
}

void Column::init(AttrType attr_type, int attr_len, size_t capacity)
{
  reset();
  data_        = new char[capacity * attr_len];
  memset(data_, 0, capacity * attr_len);
  count_       = 0;
  capacity_    = capacity;
  own_         = true;
  attr_type_   = attr_type;
  attr_len_    = attr_len;
  column_type_ = Type::NORMAL_COLUMN;
}

void Column::init(const Value &value, size_t size)
{
  reset();
  attr_type_ = value.attr_type();
  attr_len_  = value.length();
  if (attr_len_ == 0) {
    // 常量列允许逻辑长度为 0，但底层至少留 1 字节避免空指针语义混乱。
    data_      = new char[1];
    data_[0] = '\0';
  } else {
    data_      = new char[attr_len_];
  }
  count_     = size;
  capacity_  = 1;
  own_       = true;
  memcpy(data_, value.data(), attr_len_);
  column_type_ = Type::CONSTANT_COLUMN;
}

void Column::reset()
{
  if (vector_buffer_ != nullptr) {
    // VectorBuffer 只托管长文本载荷，丢弃 unique_ptr 即可统一释放整块内存。
    vector_buffer_ = nullptr;
  }
  if (data_ != nullptr && own_) {
    delete[] data_;
  }
  data_      = nullptr;
  count_     = 0;
  capacity_  = 0;
  own_       = false;
  attr_type_ = AttrType::UNDEFINED;
  attr_len_  = -1;
}

RC Column::append_one(const char *data) { return append(data, 1); }

RC Column::append(const char *data, int count)
{
  if (!own_) {
    // 引用列没有扩容和写入权限，避免误写共享缓冲区。
    LOG_WARN("append data to non-owned column");
    return RC::INTERNAL;
  }
  if (count_ + count > capacity_) {
    // 当前列不支持自动扩容；容量规划由调用方在批次创建阶段完成。
    LOG_WARN("append data to full column");
    return RC::INTERNAL;
  }
  // Using a larger integer type to avoid overflow
  size_t total_bytes = static_cast<size_t>(count) * static_cast<size_t>(attr_len_);

  memcpy(data_ + count_ * attr_len_, data, total_bytes);
  count_ += count;
  return RC::SUCCESS;
}

RC Column::append_value(const Value &value)
{
  if (!own_) {
    LOG_WARN("append data to non-owned column");
    return RC::INTERNAL;
  }
  if (count_ >= capacity_) {
    LOG_WARN("append data to full column");
    return RC::INTERNAL;
  }

  size_t total_bytes = std::min(value.length(), attr_len_);
  memcpy(data_ + count_ * attr_len_, value.data(), total_bytes);
  if (total_bytes < attr_len_) {
    // CHAR 类字段允许源值短于目标列宽，这里补终止字符方便后续字符串读取。
    data_[count_ * attr_len_ + total_bytes] = 0;
  }

  count_ += 1;
  return RC::SUCCESS;
}

string_t Column::add_text(const char *data, int length)
{
  if (vector_buffer_ == nullptr) {
    // 仅在首次写入长文本时创建附属缓冲区，避免普通定长列的额外开销。
    vector_buffer_ = make_unique<VectorBuffer>();
  }
  return vector_buffer_->add_string(data, length);
}

Value Column::get_value(int index) const
{
  if (column_type_ == Type::CONSTANT_COLUMN) {
    // 常量列只物理存储一份值，任意逻辑行都回退到索引 0。
    index  = 0;
  }
  if (index >= count_ || index < 0) {
    return Value();
  }
  return Value(attr_type_, &data_[index * attr_len_], attr_len_);
}

void Column::reference(const Column &column)
{
  if (this == &column) {
    return;
  }
  reset();

  // 这里只借用源列的连续缓冲区，不复制数据，也不继承其释放责任。
  this->data_     = column.data();
  this->capacity_ = column.capacity();
  this->count_    = column.count();
  this->own_      = false;

  this->column_type_ = column.column_type();
  this->attr_type_   = column.attr_type();
  this->attr_len_    = column.attr_len();
}
