#ifndef OTCETERA_TREE_UTIL_H
#define OTCETERA_TREE_UTIL_H
// Very simple functions that operate on trees or treenode
//	without requiring iteration (contrast w/tree_operations.h)
// Depends on: tree.h 
// Depended on by: tree_iter.h tree_operation.h 

#include "otc/otc_base_includes.h"
#include "otc/tree.h"

namespace otc {

template<typename T>
bool isInternalNode(const RootedTreeNode<T> & nd);


template<typename T>
inline bool isInternalNode(const RootedTreeNode<T> & nd) {
	return !nd.IsTip();
}

} // namespace otc
#endif
