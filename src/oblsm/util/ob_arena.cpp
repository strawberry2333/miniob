// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "oblsm/util/ob_arena.h"

namespace oceanbase {

// 初始时 Arena 还没有托管任何外部分配的内存块。
ObArena::ObArena() : memory_usage_(0) {}

ObArena::~ObArena()
{
  // Arena 生命周期结束时统一释放所有挂在名下的内存块。
  for (size_t i = 0; i < blocks_.size(); i++) {
    delete[] blocks_[i];
  }
}

}  // namespace oceanbase
