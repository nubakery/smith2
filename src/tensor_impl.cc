//
// SMITH2 - Automated Code Generation Program for General Order CC Programs.
// Filename: tensor_impl.cc
// Copyright (C) 2010 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the SMITH2 package (to be renamed).
//
// The SMITH2 package is free software; you can redistribute it and\/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SMITH2 package is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SMITH2 package; see COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <cassert>
#include <algorithm>
#include <iostream>
#include <src/tensor.h>

using namespace std;
using namespace smith2;

size_t Tensor::memory_size_needed() const {
  // one can do either with smartindex or permutable
  list<Index> indexlist = indexinfo_->smartindices()->indexlist();

  // checking...
  assert(indexlist.size() == rank_);
  
  // then calculate
  size_t out = 1lu;
  for (auto iter = indexlist.begin(); iter != indexlist.end(); ++iter)
    out *= iter->max_block_size();

  return out;
}

