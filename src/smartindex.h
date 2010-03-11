//
// Author:: Toru Shiozaki
// Date  :: Feb 2009
//
#ifndef _smith_smartindex_h
#define _smith_smartindex_h

#include <string>
#include <list>
#include <vector>
#include <src/index.h>
#include <src/cost.h>
#include <src/tensor.h>
#include <boost/shared_ptr.hpp>

class Tensor;
class SmartIndexList;

class SmartIndex {

  friend class Tensor;
  friend class Block;
  friend class Index;
  friend class SmartIndexList;

  protected:
    std::list<Index> indices_; 

    boost::shared_ptr<Tensor> my_tensor_; 
    boost::shared_ptr<Tensor> target_tensor_;

  public:
    SmartIndex(const std::list<Index>, boost::shared_ptr<Tensor>, boost::shared_ptr<Tensor>);
    ~SmartIndex();

    bool operator==(const SmartIndex& o) const;
    bool operator!=(const SmartIndex& o) const {return !((*this) == o);};
    bool operator<(const SmartIndex& o) const; 
    bool operator>(const SmartIndex& o) const { return !((*this) < o); }; // no equal here.

    /// Returns a copy of indices.
    const std::list<Index> indices() const { return indices_;}; 
    /// Returns "h", "p", or "P"
    const std::string type() const {return indices_.front().type();};
    /// Returns whether this object has dagger or not
    const bool dagger() const {return indices_.front().dagger();};
    /// Returns the number of indices in this SmartIndex
    const int length() const {return indices_.size();};

    /// Returns the pointer to the tensor in which this SmartIndex is attached.
    boost::shared_ptr<Tensor> my_tensor() const {return my_tensor_;};
    /// Returns the pointer to the tensor to which this SmartIndex will be contracted. 
    boost::shared_ptr<Tensor> target_tensor() const {return target_tensor_;};
    /// Renew target_tensor
    void set_target_tensor(boost::shared_ptr<Tensor> n) {target_tensor_ = n; };

    /// returns whether two SmartIndex can be seen as identical in the factorization step
    const bool identical(const SmartIndex&) const;
    /// Returns whether two SmartIndex is permutable or not.
    const bool permutable(const SmartIndex&) const;
    /// Merge two permutable indices.
    void merge(SmartIndex&);
    /// Undo merge (all the Smartindex will be size=1)
    const std::list<SmartIndex> extract() const;
    /// For printing out.
    const std::string show() const;
    /// Add a contribution to the computational cost to PCost 
    void add_to_pcost(PCost&);

    /// Returns the "num" of the first index
    const int num() const { return indices().front().num(); }; 
    /// Returns the pointers of "num" of indices 
    std::vector<boost::shared_ptr<int> > num_pointers(const int dagger = 0);
    /// Returns the values (copy) of "num" of indices 
    const std::vector<int> num_values(const int dagger = 0) const;

    /// Sort indices..
    void sort() { indices_.sort(); };

    ////////////////////////////////////////////
    /// for implementation
    const std::vector<std::vector<Block> > blocklist() const;

};

#endif