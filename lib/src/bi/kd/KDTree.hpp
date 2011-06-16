/**
 * @file
 *
 * @author Lawrence Murray <lawrence.murray@csiro.au>
 * $Rev$
 * $Date$
 *
 * Imported from dysii 1.4.0, originally indii/ml/aux/KDTree.hpp
 */
#ifndef BI_KD_KDTREE_HPP
#define BI_KD_KDTREE_HPP

#include "KDTreeNode.hpp"
#include "MedianPartitioner.hpp"

#ifndef __CUDACC__
#include "boost/serialization/split_member.hpp"
#endif

namespace bi {
/**
 * \f$kd\f$ (k-dimensional) tree over a weighted sample set.
 *
 * @ingroup kd
 *
 * @tparam M1 Vector type.
 */
template <class V1 = host_vector<>, class M1 = host_matrix<> >
class KDTree {
public:
  /**
   * Node type.
   */
  typedef KDTreeNode<V1,M1> node_type;

  /**
   * Default constructor.
   *
   * This should generally only be used when the object is to be
   * restored from a serialization.
   */
  KDTree();

  /**
   * Constructor.
   *
   * @tparam M2 Matrix type.
   * @tparam S1 #concept::Partitioner type.
   *
   * @param X Samples.
   * @param lw Log-weights.
   * @param partitioner Partitioner.
   */
  template<class M2, class V2, class S1>
  KDTree(const M2 X, const V2 lw, S1 partitioner);

  /**
   * Constructor.
   *
   * @tparam M2 Matrix type.
   * @tparam S1 #concept::Partitioner type.
   *
   * @param X Samples.
   * @param partitioner Partitioner.
   */
  template<class M2, class S1>
  KDTree(const M2 X, S1 partitioner);

  /**
   * Copy constructor.
   */
  KDTree(const KDTree<V1,M1>& o);

  /**
   * Destructor.
   */
  ~KDTree();

  /**
   * Assignment operator.
   */
  KDTree<V1,M1>& operator=(const KDTree<V1,M1>& o);

  /**
   * Get root node.
   *
   * @return Root node.
   */
  node_type* getRoot();

  /**
   * Get size.
   *
   * @return Size.
   */
  int getSize();

  /**
   * Set root node.
   *
   * @param root Root node.
   */
  void setRoot(node_type* root);

private:
  /**
   * Root node of the tree.
   */
  node_type* root;

  /**
   * Build \f$kd\f$ tree node.
   *
   * @tparam M2 Matrix type.
   *
   * @param X Samples.
   * @param is Indices of the subset of components in @p X over which to build
   * the node.
   * @param depth Depth of the node in the tree. Zero for the root node.
   * 
   * @return The node. Caller has ownership.
   */
  template<class M2, class V2, class S1>
  static node_type* build(const M2 X, const V2 lw, S1 partitioner,
      const std::vector<int>& is, const int depth = 0);

  #ifndef __CUDACC__
  /**
   * Serialize.
   */
  template<class Archive>
  void save(Archive& ar, const int version) const;

  /**
   * Restore from serialization.
   */
  template<class Archive>
  void load(Archive& ar, const int version);

  /*
   * Boost.Serialization requirements.
   */
  BOOST_SERIALIZATION_SPLIT_MEMBER()
  friend class boost::serialization::access;
  #endif
};
}

#include "partition.hpp"

template<class V1, class M1>
bi::KDTree<V1,M1>::KDTree() : root(NULL) {
  //
}

template<class V1, class M1>
template<class M2, class V2, class S1>
bi::KDTree<V1,M1>::KDTree(const M2 X, const V2 lw, const S1 partitioner) {
  std::vector<int> is(X.size1());
  thrust::sequence(is.begin(), is.end(), 0);

  root = (is.size() > 0) ? build(X, lw, partitioner, is) : NULL;
}

template<class V1, class M1>
template<class M2, class S1>
bi::KDTree<V1,M1>::KDTree(const M2 X, const S1 partitioner) {
  V1 lw(X.size1());
  lw.clear();
  std::vector<int> is(X.size1());
  thrust::sequence(is.begin(), is.end(), 0);

  root = (is.size() > 0) ? build(X, lw, partitioner, is) : NULL;
}

template<class V1, class M1>
bi::KDTree<V1,M1>::KDTree(const KDTree<V1,M1>& o) {
  root = (o.root == NULL) ? NULL : new KDTreeNode<V1,M1>(o.root);
}

template<class V1, class M1>
bi::KDTree<V1,M1>::~KDTree() {
  delete root;
}

template<class V1, class M1>
bi::KDTree<V1,M1>& bi::KDTree<V1,M1>::operator=(const KDTree<V1,M1>& o) {
  delete root;
  root = (o.root == NULL) ? NULL : new node_type(o.root);
  
  return *this;
}

template<class V1, class M1>
inline bi::KDTreeNode<V1,M1>* bi::KDTree<V1,M1>::getRoot() {
  return root;
}

template<class V1, class M1>
inline void bi::KDTree<V1,M1>::setRoot(node_type* root) {
  this->root = root;
}

template<class V1, class M1>
inline int bi::KDTree<V1,M1>::getSize() {
  return this->root->getSize();
}

template<class V1, class M1>
template<class M2, class V2, class S1>
typename bi::KDTree<V1,M1>::node_type* bi::KDTree<V1,M1>::build(const M2 X,
    const V2 lw, S1 partitioner, const std::vector<int>& is, const int depth) {
  /* pre-condition */
  assert (is.size() > 0);

  node_type* result;
  int i;
  
  if (is.size() == 1) {
    /* leaf node */
    result = new node_type(X, lw, is.front(), depth);
  } else if (is.size() <= 4) {
    /* prune node */
    result = new node_type(X, lw, is, depth);
  } else {
    /* internal node */
    if (partitioner.init(X, is)) {
      node_type *left, *right; // child nodes
      std::vector<int> ls, rs; // indices of left, right components
      ls.reserve(is.size()/2 + 1);
      rs.reserve(is.size()/2 + 1);
      
      for (i = 0; i < (int)is.size(); ++i) {
        if (partitioner.assign(row(X,is[i])) == LEFT) {
          ls.push_back(is[i]);
        } else {
          rs.push_back(is[i]);
        }
      }

      if (ls.size() == 0) {
        /* degenerate case left child, make prune node from right */
        result = new node_type(X, lw, rs, depth);
      } else if (rs.size() == 0) {
        /* degenerate right child, make prune node from left*/
        result = new node_type(X, lw, ls, depth);
      } else {
        /* internal node */
        left = build(X, lw, partitioner, ls, depth + 1);
        right = build(X, lw, partitioner, rs, depth + 1);
      
        result = new node_type(left, right, depth);
      }
    } else {
      /* Degenerate case, usually occurs when all points are identical or
         one has negligible weight, so that they cannot be partitioned
         spatially. Put them all into one prune node... */
      result = new node_type(X, lw, is, depth);
    }
  }

  return result;
}

#ifndef __CUDACC__
template<class V1, class M1>
template<class Archive>
void bi::KDTree<V1,M1>::save(Archive& ar, const int version) const {
  const bool haveRoot = (root != NULL);
  ar & haveRoot;
  if (haveRoot) {
    ar & root;
  }
}

template<class V1, class M1>
template<class Archive>
void bi::KDTree<V1,M1>::load(Archive& ar, const int version) {
  bool haveRoot = false;
  
  if (root != NULL) {
    delete root;
    root = NULL;
  }
  
  ar & haveRoot;
  if (haveRoot) {
    ar & root;
  }
}
#endif
#endif
