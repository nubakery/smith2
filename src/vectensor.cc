//
// Author:: Toru Shiozaki
// Date  :: Feb 2009
//
#include <cassert>
#include <algorithm>
#include <iostream>
#include <src/vectensor.h>
#include <src/cost.h>

using namespace std;
using namespace boost;

VecTensor::VecTensor(const vector<shared_ptr<Tensor> >& tensr): tensor_(tensr) {

}


VecTensor::~VecTensor() {

}


const string VecTensor::show() const {
  string out;
  vector<shared_ptr<Tensor> >::const_iterator titer;
  for (titer = tensor_.begin(); titer != tensor_.end(); ++titer) 
    out += (*titer)->show() + " ";
  out.erase(out.size()-1,1);
  return out;
}


void VecTensor::strength_reduction(const bool opt_memory) {

  vector<shared_ptr<Tensor> > perm_tensor = tensor();
  sort(perm_tensor.begin(), perm_tensor.end());
  vector<shared_ptr<Tensor> > save_tensor;
  const int length = perm_tensor.size();
  const bool notone = length > 1;

  int perm_length = 1;
  for (int i = 0; i < length; ++i) perm_length *= i + 1;

  Cost tmp;
  const int large = 100;
  if (notone) tmp.add_pcost(0, large, 0); /// must be large enough
  pair<Cost, Cost> keep_ = make_pair(tmp, tmp);

  while(1) { 

    bool redundant = false;
    if (notone) {
      const Tensor a0 = *(perm_tensor[0]);
      const Tensor a1 = *(perm_tensor[1]);
      redundant = a0 > a1;
    }

    if (notone && !redundant) {

      pair<Cost, Cost> eval = cost_evaluater(perm_tensor);
      if (!opt_memory) {
        if (eval.first < keep_.first) {
          keep_ = eval;
          save_tensor = perm_tensor;
        } else if (eval.first == keep_.first && eval.second < keep_.second) {
          keep_ = eval;
          save_tensor = perm_tensor;
        }
      } else {
        if (eval.second < keep_.second) {
          keep_ = eval;
          save_tensor = perm_tensor;
        } else if (eval.second == keep_.second && eval.first < keep_.first) {
          keep_ = eval;
          save_tensor = perm_tensor;
        }
      }
      
    }
    if(!next_permutation(perm_tensor.begin(), perm_tensor.end())) break;
  }

  if (notone) {
    reverse(save_tensor.begin(), save_tensor.end());
    tensor_ = save_tensor; 
    tensor_ = save_tensor; 
  }
  refresh_tensor_depth();
  refresh_indices();
  refresh_sign();
  refresh_factor();

  cout << endl;
  cout << "  " << show() << endl;
  cout << "  Operation: " << keep_.first.show() << "  Memory: " << keep_.second.show() << endl << endl;

  
  
}


const pair<Cost, Cost> VecTensor::cost_evaluater(const vector<shared_ptr<Tensor> > tensors) const {

  list<SmartIndex> current;
  vector<shared_ptr<Tensor> >::const_iterator titer;
  vector<shared_ptr<Tensor> > finished;
  Cost cop;
  Cost cmem; 
  for (titer = tensors.begin(); titer != tensors.end(); ++titer) {
    if (titer != tensors.begin()) {
      // evaluate cost here
      PCost pcop(0,0,0);
      PCost pcmem(0,0,0);
      list<SmartIndex> next; 

      list<SmartIndex>::iterator siter;
      for (siter = current.begin(); siter != current.end(); ++siter) {
        if (siter->target_tensor().get() != titer->get()) { // if not contraction indices
          next.push_back(*siter);
          siter->add_to_pcost(pcmem);
          siter->add_to_pcost(pcop);
        } 
      }
      list<SmartIndex> newlist = (*titer)->smartindices()->si();
      for (siter = newlist.begin(); siter != newlist.end(); ++siter) {
        siter->add_to_pcost(pcop);
        if (!siter->target_tensor() || !siter->target_tensor()->in_list(finished)) { // if not contraction indices 
          next.push_back(*siter);
          siter->add_to_pcost(pcmem);
        }
      }

      cop.add_pcost(pcop);
      cmem.add_pcost(pcmem);

      current = next;
    } else { 
      current = (*titer)->smartindices()->si();
    }
    finished.push_back(*titer);
  }
  cop.sort_pcost();
  cmem.sort_pcost();
  pair<Cost, Cost> out = make_pair(cop, cmem);


  return out;
}


void VecTensor::refresh_tensor_depth() {
  vector<boost::shared_ptr<Tensor> >::iterator titer;
  int depth = 0;
  for (titer = tensor_.begin(); titer != tensor_.end(); ++titer, ++depth)
    (*titer)->set_depth(depth);
}


void VecTensor::refresh_indices() {
/// this function is driven by the pointers to num of indices... 
/// pretty complicated; but works. // info for number matching will be
/// storeted in map<shared_ptr<int>, vector<shared_ptr<int> > >

  vector<shared_ptr<Tensor> >::iterator titer;

  map<shared_ptr<int> , vector<shared_ptr<int> > > dupl;
  vector<shared_ptr<int> > eh, ep, ecp; 
  vector<pair<shared_ptr<int> , shared_ptr<Tensor> > > ih, ip, icp; // internal
  for (titer = tensor_.begin(); titer != tensor_.end(); ++titer) { 
    shared_ptr<Tensor> current_tensor = *titer;
    current_tensor->sort_indices();

    list<SmartIndex>::iterator siter;
    list<SmartIndex>* smartindices = current_tensor->smartindices_pointer(); 
    for (siter = smartindices->begin(); siter != smartindices->end(); ++siter) {
      SmartIndex* current_si = &*siter;

      if (current_si->target_tensor().get() == NULL) {
        if (current_si->type() == "h") { 
          vector<shared_ptr<int> > tmp = current_si->num_pointers(); 
          eh.insert(eh.end(), tmp.begin(), tmp.end());
        } else if (current_si->type() == "p") {
          vector<shared_ptr<int> > tmp = current_si->num_pointers(); 
          ep.insert(ep.end(), tmp.begin(), tmp.end());
        } else if (current_si->type() == "P") {
          vector<shared_ptr<int> > tmp = current_si->num_pointers(); 
          ecp.insert(ecp.end(), tmp.begin(), tmp.end());
        }
      } else {
       // internal indices come here
        vector<shared_ptr<int> >::iterator iiter;
        vector<shared_ptr<int> > tmp = current_si->num_pointers(); 
        vector<pair<shared_ptr<int> , shared_ptr<Tensor> > > iveci;
        if (current_si->type() == "h")  iveci =  ih;
        else if (current_si->type() == "p") iveci =  ip;
        else if (current_si->type() == "P") iveci =  icp;
        for (iiter = tmp.begin(); iiter != tmp.end(); ++iiter) { 
          bool find_ = false;
          for (vector<pair<shared_ptr<int> , shared_ptr<Tensor> > >::iterator i = iveci.begin(); i != iveci.end(); ++i) { 
            if (*((*i).first) == *(*iiter)) { 
              map<shared_ptr<int> , vector<shared_ptr<int> > >::iterator mapiter = dupl.find((*i).first);
              if (mapiter == dupl.end()) {
                vector<shared_ptr<int> > tmpv(1, *iiter);
                dupl.insert(make_pair((*i).first, tmpv));
              } else {
                (*mapiter).second.push_back(*iiter);
              }
              find_ = true;
              break;
            }
          }
          if (!find_) iveci.push_back(make_pair(*iiter, *titer));
        }
        
        if (current_si->type() == "h")  ih = iveci;
        else if (current_si->type() == "p") ip =  iveci;
        else if (current_si->type() == "P") icp =  iveci;
      }
    }
  }

  int count = 1;
  {
    vector<shared_ptr<int> >::iterator iiter;
    /// numbering external indices
    for (iiter = eh.begin(); iiter != eh.end(); ++iiter, ++count)   {*(*iiter) = count;} 
    for (iiter = ep.begin(); iiter != ep.end(); ++iiter, ++count)   {*(*iiter) = count;}
    for (iiter = ecp.begin(); iiter != ecp.end(); ++iiter, ++count) {*(*iiter) = count;}
  }
  
    /// numbering internal indices
  for (titer = tensor_.begin(); titer != tensor_.end(); ++titer) 
  {
    vector<pair<shared_ptr<int> , shared_ptr<Tensor> > >::iterator iiter;
    for (iiter = ih.begin(); iiter != ih.end(); ++iiter)   {
      if (*titer != (*iiter).second) continue;
  
      *((*iiter).first) = count;
      map<shared_ptr<int> , vector<shared_ptr<int> > >::iterator mapiter = dupl.find((*iiter).first);
      if (mapiter != dupl.end()) {
        for (vector<shared_ptr<int> >::iterator ii = (*mapiter).second.begin(); ii != (*mapiter).second.end(); ++ii) *(*ii) = count;
      }
      ++count;
    }
    for (iiter = ip.begin(); iiter != ip.end(); ++iiter)   {
      if (*titer != (*iiter).second) continue;
  
      *((*iiter).first) = count;
      map<shared_ptr<int> , vector<shared_ptr<int> > >::iterator mapiter = dupl.find((*iiter).first);
      if (mapiter != dupl.end()) {
        for (vector<shared_ptr<int> >::iterator ii = (*mapiter).second.begin(); ii != (*mapiter).second.end(); ++ii) *(*ii) = count;
      }
      ++count;
    }
    for (iiter = icp.begin(); iiter != icp.end(); ++iiter) {
      if (*titer != (*iiter).second) continue;
  
      *((*iiter).first) = count;
      map<shared_ptr<int> , vector<shared_ptr<int> > >::iterator mapiter = dupl.find((*iiter).first);
      if (mapiter != dupl.end()) {
        for (vector<shared_ptr<int> >::iterator ii = (*mapiter).second.begin(); ii != (*mapiter).second.end(); ++ii) *(*ii) = count;
      }
      ++count;
    }
  }

}


void VecTensor::refresh_sign() {
/// sign will be put to the last tensor
  const int numloops = count_loops(); 
  const int numholes = count_holes();

  const double factor = ((numloops + numholes) % 2 == 0) ? 1.0 : -1.0;
  tensor_pointer()->back()->set_factor(factor); 
}


void VecTensor::refresh_factor() {

  /// TODO currently I only perform the tensor-tensor matching that is 
  /// sufficient for the CC-R12 family of methods. In more general cases,
  /// we need to account for i) tensors-tensors matching, 
  /// ii) 3-, 4-, .. folded symmetry etc. Those will be easy. 
 
  double factor = 1.0;

  vector<shared_ptr<Tensor> > tensors = tensor();
  vector<shared_ptr<Tensor> >::iterator titer, titer2, tmp, tmpiter; 

  for (titer = tensors.begin(); titer != tensors.end(); ++titer) { 
    tmp = titer;
    for (titer2 = (++tmp); titer2 != tensors.end(); ++titer2) {

      /// most of the time, **titer == **titer2 is sufficient
      /// but in case if **titer has an index that point **titer and those are
      /// apart in the VecTensor exression, that will be a problem.
      /// To avoid this, another vector is set up below.
      vector<shared_ptr<Tensor> > v;       
//    Tensor v0(*(*titer)), v1(*(*titer2));
      v.push_back(*titer);
      v.push_back(*titer2);
      for (tmpiter = tensors.begin(); tmpiter != tensors.end();  ++tmpiter) {
        if (tmpiter != titer && tmpiter != titer2) v.push_back(*tmpiter);
      }
      (*titer)->sort_indices();
      (*titer2)->sort_indices();
      if ( *(v.at(0)) == *(v.at(1)) ) 
        factor *= 0.5;
    }
  }

  tensor_pointer()->back()->mul_factor(factor);
}


const int VecTensor::count_loops() {
  int out = 0;
  map<int, int> indexpair = index_num_pair();

  while (!indexpair.empty()) {

    map<int, int>::iterator a = indexpair.begin();
    vector<map<int, int>::iterator> used(1, a);
    const int target = (*a).first;
    int current = (*a).second; 

    while (current != target) {
      a = indexpair.find(current); 
      assert(a != indexpair.end());

      current = (*a).second;
      used.push_back(a);
    }

    for (vector<map<int, int>::iterator>::iterator mi = used.begin(); mi != used.end(); ++mi)
      indexpair.erase(*mi);

    ++out;
  }

  return out;
}


const int VecTensor::count_holes() {
  vector<shared_ptr<Tensor> > tensors = tensor();
  vector<int> temp;
  for (vector<shared_ptr<Tensor> >::iterator titer = tensors.begin(); titer != tensors.end(); ++titer) {
    vector<int> holelines = (*titer)->num_values_hole(); 

    for (vector<int>::iterator iiter = holelines.begin(); iiter != holelines.end(); ++iiter) { 
      if (find(temp.begin(), temp.end(), *iiter) == temp.end())
        temp.push_back(*iiter);
    }

  }
  return temp.size();
}


const map<int, int> VecTensor::index_num_pair() const {
  map<int, int> out;

  vector<shared_ptr<Tensor> > tensors = tensor(); 
  vector<int> edag;
  vector<int> endag;
  for (vector<shared_ptr<Tensor> >::iterator titer = tensors.begin(); titer != tensors.end(); ++titer) {
    vector<int> dag = (*titer)->num_values(1);
    vector<int> ndag = (*titer)->num_values(-1);
    vector<int> tmp_ed = (*titer)->num_values_external(1);
    vector<int> tmp_en = (*titer)->num_values_external(-1);
    if (!tmp_ed.empty())
      edag.insert(edag.end(), tmp_ed.begin(), tmp_ed.end());
    if (!tmp_en.empty())
      endag.insert(endag.end(), tmp_en.begin(), tmp_en.end());
    assert(dag.size() == ndag.size());

    vector<int>::iterator i,j;
    for (i = dag.begin(), j = ndag.begin(); i != dag.end(); ++i, ++j) { 
      pair<map<int, int>::iterator, bool> a = out.insert(make_pair(*i, *j));
      assert(a.second);
    } 
  } 

  // probably need to modify in case of non-number-conserving theory such as IP and EA
  assert(edag.size() == endag.size());
  vector<int>::iterator i, j;
  for (i = edag.begin(), j = endag.begin(); i != edag.end(); ++i, ++j) {
    // adding dummy indices that makes diagrams closed
    pair<map<int, int>::iterator, bool> a = out.insert(make_pair(*j, *i));
    assert(a.second);
  }  

  return out;
}


const bool VecTensor::identical(const VecTensor& o) const {
  vector<shared_ptr<Tensor> > otensor_ = o.tensor();
  if (tensor_.size() != otensor_.size()) return false;

  vector<shared_ptr<Tensor> >::const_iterator i, j;
  for (i = tensor_.begin(), j = otensor_.begin(); i != tensor_.end(); ++i, ++j) {
    if (!((*i)->identical(*j))) return false;
  }

  return true;
}
