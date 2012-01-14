//
// Author:: Toru Shiozaki
// Date  :: Feb 2010
//
#include <cassert>
#include <algorithm>
#include <iostream>
#include <src/tensor.h>

using namespace std;

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

