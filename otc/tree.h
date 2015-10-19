#ifndef OTCETERA_TREE_H
#define OTCETERA_TREE_H

#include <climits>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include "otc/otc_base_includes.h"

namespace otc {
template<typename, typename> class RootedTree;

typedef std::string namestring_t;

template<typename T>
class RootedTreeNode {
    public:
        using node_type = RootedTreeNode<T>;
        using data_type = T;
        bool isTip() const {
            return (lChild == nullptr);
        }
        bool isInternal() const {
            return (lChild != nullptr);
        }
        const node_type * getParent() const {
            return parent;
        }
        node_type * getParent() {
            return parent;
        }
        const node_type * getFirstChild() const {
            return lChild;
        }
        node_type * getFirstChild() {
            return lChild;
        }
        const node_type * getNextSib() const {
            return rSib;
        }
        node_type * getNextSib() {
            return rSib;
        }
        // low-level
        void _setFirstChild(node_type *s) {
            lChild = s;
        }
        void _setNextSib(node_type *s) {
            rSib = s;
        }
        void _setParent(node_type *s) {
            parent = s;
        }
        //
        const node_type * getLastChild() const {
            if (lChild == nullptr)
                return nullptr;
            return (lChild->rSib == nullptr ? lChild : lChild->getLastSib());
        }
        node_type * getLastChild() {
            return const_cast<node_type *>(const_cast<const node_type *>(this)->getLastChild());
        }
        const node_type * getLastSib() const {
            auto currNode = this->getNextSib();
            if (currNode == nullptr) {
                return nullptr;
            }
            auto nextNd = currNode->getNextSib();
            while (nextNd != nullptr) {
                currNode = nextNd;
                nextNd = currNode->getNextSib();
            }
            return currNode;
        }
        node_type * getLastSib() {
            return const_cast<node_type *>(const_cast<const node_type *>(this)->getLastSib());
        }
        const node_type * getPrevSib() const {
            if (this->getParent() == nullptr) {
                return nullptr;
            }
            auto currNode = this->getParent()->getFirstChild();
            if (currNode == this) {
                return nullptr;
            }
            while (currNode->getNextSib() != this) {
                currNode = currNode->getNextSib();
                assert(currNode != nullptr);
            }
            assert(currNode->getNextSib() == this);
            return currNode;
        }
        node_type * getPrevSib() {
            return const_cast<node_type *>(const_cast<const node_type *>(this)->getPrevSib());
        }
        std::vector<node_type *> getChildrenNC() {
            std::vector<node_type *> children;
            auto * currNode = getFirstChild();
            while (currNode) {
                children.push_back(currNode);
                currNode = currNode->getNextSib();
            }
            return children;
        }
        std::vector<node_type *> getChildren() {
            return getChildrenNC();
        }
        std::vector<const node_type *> getChildren() const {
            node_type * t = const_cast<node_type *>(this);
            std::vector<const node_type *> r;
            for (auto i : t->getChildren()) {
                r.push_back(i);
            }
            return r;
        }
        unsigned getOutDegree() const {
            unsigned n = 0;
            auto currNode = getFirstChild();
            while(currNode) {
                n += 1;
                currNode = currNode->getNextSib();
            }
            return n;
        }
        bool hasOttId() const {
            return ottId != LONG_MAX;
        }
        long getOttId() const {
	    assert(hasOttId());
	    return ottId;
        }
        void setOttId(long i) {
            ottId = i;
	    assert(hasOttId());
        }
        void delOttId() {
            ottId = LONG_MAX;
        }
        // non-empty only for internals that are labelled with names that are NOT taxLabels
        const namestring_t & getName() const {
            return name;
        }
        void setName(const namestring_t &n) {
            name = n;
        }
        const T & getData() const {
            return data;
        }
        T & getData() {
            return data;
        }
        RootedTreeNode<T>(RootedTreeNode<T> *par)
            :lChild(nullptr),
            rSib(nullptr),
            parent(par),
            ottId(LONG_MAX) {
        }
        void addSib(node_type *n) {
            node_type * cs = getLastSib();
            if (cs == nullptr) {
                rSib = n;
            } else {
                cs->rSib = n;
            }
        }
        void addChild(node_type *n) {
            if (lChild) {
                lChild->addSib(n);
            } else {
                lChild = n;
            }
            n->parent = this;
        }
        bool removeChild(node_type *n) {
            if (n == nullptr || lChild == nullptr) {
                return false;
            }
            if (lChild == n) {
                lChild = lChild->rSib;
            } else {
                auto c = lChild;
                for (;;) {
                    if (c->rSib == n) {
                        c->rSib = n->rSib;
                        break;
                    }
                    if (c->rSib == nullptr) {
                        return false;
                    }
                    c = c->rSib;
                }
            }
            n->parent = nullptr;
            return true;
        }
        bool isOutDegreeOneNode() const {
            return (lChild != nullptr) && (lChild->rSib == nullptr);
        }
        bool isOutDegreeTwoNode() const {
            return ((lChild != nullptr)
                    && (lChild->rSib != nullptr)
                    && (lChild->rSib->rSib == nullptr));
        }
        bool isPolytomy() const {
            return ((lChild != nullptr)
                    && (lChild->rSib != nullptr)
                    && (lChild->rSib->rSib != nullptr));
        }
        bool includesOnlyOneLeaf() const {
            if (isTip()) {
                return true;
            }
            return isOutDegreeOneNode() && lChild->includesOnlyOneLeaf();
        }
        // takes this node out of the child array of its parent, but does not fix any other pointer.
        void _detachThisNode() {
            auto ls = getPrevSib();
            if (ls == nullptr) {
                auto p = getParent();
                if (p != nullptr) {
                    p->_setFirstChild(getNextSib());
                }
            } else {
                ls->_setNextSib(getNextSib());
            }
            rSib = nullptr;
            parent = nullptr;
       }
    public:
        void writeAsNewick(std::ostream &out,
                           bool useLeafNames,
                           const std::map<node_type *, namestring_t> *nd2name=nullptr) const {
            auto child = getFirstChild();
            if (child) {
                out << "(";
                child->writeAsNewick(out,useLeafNames,nd2name);
                child = child->getNextSib();
                for(;child;child = child->getNextSib()) {
                    out << ",";
                    child->writeAsNewick(out,useLeafNames,nd2name);
                }
                out << ")";
            }
            out << getName();
        }
        void addSelfAndDesToPreorder(std::vector<const node_type *> &p) const;
        void lowLevelSetFirstChild(node_type *nd) {
            lChild = nd;
        }
        void lowLevelSetNextSib(node_type *nd) {
            rSib = nd;
        }
    private:
        node_type * lChild;
        node_type * rSib;
        node_type * parent;
        namestring_t name; // non-empty only for internals that are labelled with names that are NOT taxLabels
        long ottId; // present for every leaf. UINT_MAX for internals labeled with taxlabels
        T data;
    private:
        RootedTreeNode<T>(const RootedTreeNode<T> &) = delete;
        RootedTreeNode<T> & operator=(const RootedTreeNode<T> &) = delete;
        template<typename Y, typename Z>
        friend class RootedTree;
};

template<typename NodeType>
const NodeType* findRoot(const NodeType* nd)
{
    while(nd->getParent())
        nd = nd->getParent();
    return nd;
}
    
template<typename T, typename U>
class RootedTree {
    public:
        using node_data_type = T;
        using node_type = RootedTreeNode<T>;
        using data_type = U;
        
        RootedTree<T, U>()
            :root(nullptr) {
        }
        ~RootedTree<T, U>() {
            clear();
        }
        std::vector<const node_type *> getPreorderTraversal() const;
        void writeAsNewick(std::ostream &out,
                           bool useLeafNames,
                           const std::map<node_type *, namestring_t> *nd2name=nullptr) const {
            if (root) {
                root->writeAsNewick(out, useLeafNames, nd2name);
            }
        }
        const node_type * getRoot() const {
            return root;
        }
        node_type * getRoot() {
            return root;
        }
        void _setRoot(node_type * r) {
            root = r;
        }
        node_type * createRoot() {
            if (root != nullptr) {
                clear();
            }
            this->root = this->allocNewNode(nullptr);
            return this->root;
        }
        node_type * createChild(node_type *par) {
            auto c = this->allocNewNode(par);
            par->addChild(c);
            return c;
        }
        node_type * createNode(node_type *par) {
            auto c = this->allocNewNode(par);
            if (par != nullptr) {
                par->addChild(c);
            }
            return c;
        }
        node_type * createSib(node_type *leftSib) {
            assert(leftSib->parent != nullptr);
            auto s = this->allocNewNode(leftSib->parent);
            leftSib->addSib(s);
            return s;
        }
        U & getData() {
            return this->data;
        }
        const U & getData() const {
            return this->data;
        }
        void _pruneAndDangle(node_type * nd) {
            assert(findRoot(nd) == root);
            auto p = nd->getParent();
            if (p == nullptr) {
                root = nullptr;
                return;
            }
            p->removeChild(nd);
        }
        void _pruneAndDelete(node_type * nd) {
            auto nodes = getSubtreeNodes(nd);
            _pruneAndDangle(nd);
            for(auto nd: nodes)
                delete nd;
        }
        bool isDetached(node_type * nd) {
            return contains(detached, nd);
        }
    protected:
        node_type * root;
        U data;
        std::string name;
        std::set<node_type *> detached;
        
    public:
        void setName(const std::string &n) {
            name.assign(n);
        }
        const std::string & getName() const {
            return name;
        }
        node_type * allocNewNode(node_type *p) {
            node_type * nd = new node_type(p);
            return nd;
        }
        void clear() {
            for(auto nd: getAllAttachedNodes())
                delete nd;

            root = NULL;
        }
        std::vector<const node_type *> getAllAttachedNodes() const {
            return getSubtreeNodes(root);
        }
          
        std::vector<const node_type *> getSubtreeNodes(node_type* p) const {
            std::vector<const node_type*> nodes;
            if (p) {
                nodes.push_back(p);
            }
            for(std::size_t i = 0; i < nodes.size(); i++) {
                for(auto n = nodes[i]->lChild; n; n = n->rSib) {
                    nodes.push_back(n);
                }
            }
            return nodes;
        }
        std::set<const node_type *> getSetOfAllAttachedNodes() const {
            std::set<const node_type*> nodes;
            for(auto nd: getAllAttachedNodes())
              nodes.insert(nd);
            return nodes;
        }
        void markAsDetached(node_type * nd) {
            detached.insert(nd);
        }
        void markAsAttached(node_type * nd) {
          detached.erase(nd);
        }
    private:
        RootedTree<T, U>(const RootedTree<T, U> &) = delete;
        RootedTree<T, U> & operator=(const RootedTree<T, U> &) = delete;
};

class RTNodeNoData{};
class RTreeNoData{};

template<typename Tree>
void addSubtree(typename Tree::node_type* par, Tree& T2)
{
    auto c = T2.getRoot();
    T2._pruneAndDangle(c);
    par->addChild(c);
}

template<typename Tree>
void replaceWithSubtree(typename Tree::node_type* n, Tree& T2)
{
    // Get the parent of the tip we are replacing
    auto p = n->getParent();
    // Remove the data from T2 and attach it to this parent
    auto c = T2.getRoot();
    T2._pruneAndDangle(c);
    p->addChild(c);
    // Remove the old child from under p
    p->removeChild(n);
    // Make the old child the root of T2, which will handle deleting it
    T2._setRoot(n);
}

} // namespace otc
#endif

