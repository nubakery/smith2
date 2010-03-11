//
// Author:: Toru Shiozaki
// Date  :: Feb 2009
//
#include <cassert>
#include <algorithm>
#include <iostream>
#include <src/tensor.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;


Tensor::Tensor(const string symb, const list<SmartIndex> sindx, const int dep, const double fac)
  : symbol_(symb), depth_(dep), factor_(fac) {

  {
    shared_ptr<SmartIndexList> tmp(new SmartIndexList(sindx));
    indexinfo_->smartindices() = tmp;
  }

  rank_ = 0;
  for (list<SmartIndex>::const_iterator iter = indexinfo_->smartindices()->begin();
                                        iter != indexinfo_->smartindices()->end(); ++iter) 
    rank_ += iter->length();

  set_permutables_for_input(sindx);

  index_initialized_ = true;

}


Tensor::Tensor(const string symb, const int dep, const double fac) 
  : symbol_(symb), depth_(dep), factor_(fac) {

/// this will be used in PreTensor::analyze()
  index_initialized_ = false;

}


Tensor::Tensor(const Tensor& a) 
  : symbol_(a.symbol()), depth_(a.depth()), factor_(a.factor()), rank_(a.rank()), 
    index_initialized_(a.index_initialized()), regtensors_(a.regtensors())  { 

  list<SmartIndex> si = a.smartindices()->si();
  boost::shared_ptr<SmartIndexList> sil_si(new SmartIndexList(si));
  list<SmartIndex> perm = a.permutables()->si();
  boost::shared_ptr<SmartIndexList> sil_perm(new SmartIndexList(perm));

  boost::shared_ptr<IndexInfo> tmp(new IndexInfo(sil_si, sil_perm));
  indexinfo_ = tmp; 

}


Tensor::~Tensor() {

}


bool Tensor::operator==(const Tensor& other) const {
  if (symbol() != other.symbol()) return false;
  list<SmartIndex>::iterator i, j;
  list<SmartIndex> mine  = smartindices()->si();
  list<SmartIndex> yours = other.smartindices()->si();

  if(mine.size() != yours.size()) return false;

  for (i = mine.begin(), j = yours.begin(); i != mine.end(); ++i, ++j) { 
    if (*i != *j) return false;
  }

  return true;
}


bool Tensor::operator<(const Tensor& other) const {
// general tensor must be "smaller"
  const bool i_am_general    = symbol() == "v"       || symbol() == "f";
  const bool you_are_general = other.symbol() == "v" || other.symbol() == "f";
  if      (i_am_general    && !you_are_general) return true;
  else if (you_are_general && !i_am_general)    return false;

// we don't have general tensors. 
// Next, left operator must be "smaller".
  const bool i_am_left    = symbol() == "l"       || symbol() == "L"; 
  const bool you_are_left = other.symbol() == "l" || other.symbol() == "L"; 
  if      (i_am_left    && !you_are_left) return true;
  else if (you_are_left && !i_am_left)    return false;

// Next, the special tensors must be smaller
  const bool i_am_special    = symbol() == "V" || symbol() == "X"
                            || symbol() == "B" || symbol() == "P"; 
  const bool you_are_special = other.symbol() == "V" || other.symbol() == "X" 
                            || other.symbol() == "B" || other.symbol() == "P"; 
  if      (i_am_special    && !you_are_special) return true;
  else if (you_are_special && !i_am_special)    return false;

// Next, right operator must be "smaller".
  const bool i_am_right    = symbol() == "s"       || symbol() == "S"; 
  const bool you_are_right = other.symbol() == "s" || other.symbol() == "S"; 
  if      (i_am_right    && !you_are_right) return true;
  else if (you_are_right && !i_am_right)    return false;
  
  if ( symbol() == other.symbol() ) {
    if ( rank() != other.rank() ) 
      return rank() < other.rank();  
  }
  else
    return symbol() < other.symbol();

  return this < &other;
}


void Tensor::index_init(const list<SmartIndex> sindx) {
  assert(!index_initialized_);

  {
    boost::shared_ptr<SmartIndexList> tmp(new SmartIndexList(sindx));
    boost::shared_ptr<IndexInfo> tmp2(new IndexInfo(tmp));
    indexinfo_ = tmp2;
  }
  rank_ = 0;
  for (list<SmartIndex>::iterator iter=indexinfo_->smartindices()->begin();
                                  iter!=indexinfo_->smartindices()->end(); ++iter) 
    rank_ += iter->length();

  set_permutables_for_input(sindx);
  index_initialized_ = true;

}


void Tensor::set_permutables_for_input(const list<SmartIndex>& sinp) {
  list<SmartIndex> si = sinp;
  list<SmartIndex>::iterator siter, siter2, tmp;
  vector<list<SmartIndex>::iterator> remove;

  // permutable indices are merged and registered to remove
  for (siter = si.begin(); siter != si.end(); ++siter) {
    if (find(remove.begin(), remove.end(), siter) != remove.end()) continue;
    tmp = siter;
    ++tmp;
    for (siter2 = tmp; siter2 != si.end(); ++siter2) {
      if (siter->type() == siter2->type() && siter->dagger() == siter2->dagger() &&
          siter->my_tensor() == siter2->my_tensor() ) {
        siter->merge(*siter2); 
        remove.push_back(siter2);
      }
    }
  } 

  // removing merged indices
  for (vector<list<SmartIndex>::iterator>::iterator i = remove.begin(); i != remove.end(); ++i)
    si.erase(*i); 

  // sorting indices
  si.sort();

  set_permutables(si);
}


const string Tensor::show() const {
  string out; 
  out += symbol() + "(";

  list<SmartIndex>::const_iterator siter;
  for (siter = indexinfo_->smartindices()->begin(); siter != indexinfo_->smartindices()->end(); ++siter) 
    out += siter->show() + " ";

  out += "; ";
  for (siter = indexinfo_->permutables()->begin(); siter != indexinfo_->permutables()->end(); ++siter) 
    out += siter->show() + " ";

  if (!indexinfo_->smartindices()->empty()) out.erase(out.size()-1,1);
  out += ")";
  if (factor() != 1.0)
    out += " * " + lexical_cast<string>(factor());
  return out;
}


const vector<int> Tensor::num_values(const int dagger) const {
/// dagger = 1 returns only those with dagger,
/// dagger = -1 returns only those without dagger
  vector<int> out;
  list<SmartIndex>::const_iterator siter;
  for (siter = indexinfo_->smartindices()->begin(); siter != indexinfo_->smartindices()->end(); ++siter) {
    vector<int> tmp = siter->num_values(dagger);
    out.insert(out.end(), tmp.begin(), tmp.end());    
  }
  return out;
}


const vector<int> Tensor::num_values_external(const int dagger) const {
/// dagger = 1 returns only those with dagger,
/// dagger = -1 returns only those without dagger
  vector<int> out;
  list<SmartIndex>::const_iterator siter;
  for (siter = indexinfo_->smartindices()->begin(); siter != indexinfo_->smartindices()->end(); ++siter) {
    if (siter->target_tensor() == NULL) {
      vector<int> tmp = siter->num_values(dagger);
      out.insert(out.end(), tmp.begin(), tmp.end());    
    }
  }
  return out;
}


const vector<int> Tensor::num_values_hole() const {
/// dagger = 1 returns only those with dagger,
/// dagger = -1 returns only those without dagger
  vector<int> out;
  list<SmartIndex>::const_iterator siter;
  for (siter = indexinfo_->smartindices()->begin(); siter != indexinfo_->smartindices()->end(); ++siter) {
    if (siter->type() == "h") {
      vector<int> tmp = siter->num_values(0);
      out.insert(out.end(), tmp.begin(), tmp.end());    
    }
  }
  return out;
}


const bool Tensor::identical(shared_ptr<Tensor> other) const {

  bool out = true;

  if (symbol() != other->symbol()) out = false;
  if (smartindices()->size() != other->smartindices()->size()) out = false;

  list<SmartIndex>::const_iterator i, j;
  const list<SmartIndex> mine  = smartindices()->si();
  const list<SmartIndex> yours = other->smartindices()->si();
  for (i = mine.begin(), j = yours.begin(); i != mine.end(); ++i, ++j) {
    if (!i->identical(*j)) { 
      out = false;
      break;
    }
  }  

  return out;
}


void Tensor::sort_indices() {
  indexinfo_->smartindices()->sort();
  indexinfo_->permutables()->sort();
}
