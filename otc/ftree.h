#ifndef OTCETERA_FTREE_H
#define OTCETERA_FTREE_H

#include <map>
#include <string>
#include <vector>
#include <set>
#include <list>
#include "otc/otc_base_includes.h"
#include "otc/tree.h"
#include "otc/util.h"
namespace otc {
template<typename T, typename U> class GreedyPhylogeneticForest;
template<typename T, typename U> class RootedForest;
template<typename T, typename U> class FTree;

enum ConstraintType {
    INCLUDES_NODE,
    EXCLUDES_NODE
};
struct PhyloStatementSource {
    PhyloStatementSource(int treeInd, long groupInd)
        :sourceTreeId(treeInd),
        cladeId(groupInd) {
    }
    const int sourceTreeId;
    const long cladeId; // ID of the node in the tree
};

struct PhyloStatement {
    /*PhyloStatement(const OttIdSet &includes, const OttIdSet & other, bool otherIsExcludes)
        :includeGroup(includes),
        excludeGroup(otherIsExcludes ? other : set_difference_as_set(other, includes)),
        leafSet(otherIsExcludes ? set_union_as_set(other, includes): other) {
    }*/
    PhyloStatement(const OttIdSet &includes,
                   const OttIdSet &excludes,
                   const OttIdSet &mentioned, 
                   PhyloStatementSource pss)
        :includeGroup(includes),
        excludeGroup(excludes),
        leafSet(mentioned),
        provenance(pss) {
        debugCheck();
    }
    bool debugCheck() const;
    bool isTrivial() const {
        return includeGroup.size() < 2 || includeGroup.size() == leafSet.size();
    }
    const OttIdSet & includeGroup;
    const OttIdSet & excludeGroup;
    const OttIdSet & leafSet; // just the union of the includeGroup and excludeGroup
    const PhyloStatementSource provenance;
};

// Bands connect nodes across different FTree instance when the nodes are required
//  to satisfy a grouping that has been added. The includeGroup of the ps
//  is a set of IDs that will be in the desIds of each member of the band (and
//  the ancestor nodes of the members).
//  The nodes in the ps.excludeGroup should be excluded at this node or an ancestor.
template<typename T>
class InterTreeBand {
    public:
    using node_type = RootedTreeNode<T>;
    using node_set = std::set<node_type *>;
    InterTreeBand(node_type * nd1,
                  const node_set & n1set,
                  node_type *nd2,
                  const node_set & n2set,
                  const PhyloStatement & ps)
        :statement(ps) {
        addNode(nd1, n1set);
        addNode(nd2, n2set);
    }
    void addNode(node_type * nd, const node_set & nset) {
        assert(nd != nullptr);
        const auto s = nd2phantom.size();
        nd2phantom[nd] = nset;
        assert(s == nd2phantom.size());
    }
    bool bandPointHasAny(const node_type * nd, const node_set & ns) const {
        return !(areDisjoint(nd2phantom.at(nd), ns));
    }
    OttIdSet getPhantomIds(const node_type * nd) const {
        OttIdSet r;
        auto npIt = nd2phantom.find(nd);
        if (npIt != nd2phantom.end()) {
            for (auto np : npIt->second) {
                r.insert(np->getOttId());
            }
        }
        return r;
    }
    void debugInvariantsCheckITB() const;
    private:
    std::map<const node_type *, node_set> nd2phantom;
    const PhyloStatement & statement;
};

template<typename T>
class ExcludeConstraints {
    public:
    using node_type = RootedTreeNode<T>;
    using node_pair = std::pair<const node_type *, const node_type *>;
    using cnode_set = std::set<const node_type *>;
    using node2many_map = std::map<const node_type *, cnode_set >;
    bool addExcludeStatement(const node_type * nd2Exclude, const node_type * forbiddenAttach);
    bool isExcludedFrom(const node_type * ndToCheck, const node_type * potentialAttachment) const;
    void debugInvariantsCheckEC() const;
    bool hasNodesExcludedFromIt(const node_type *n) const {
        return contains(byNdWithConstraints, n);
    }
    private:
    void purgeExcludeRaw(const node_type * nd2Exclude, const node_type * forbiddenAttach);
    void ingestExcludeRaw(const node_type * nd2Exclude, const node_type * forbiddenAttach);
    node2many_map byExcludedNd;
    node2many_map byNdWithConstraints;
};

template<typename T>
class InterTreeBandBookkeeping {
    public:
    using node_type = RootedTreeNode<T>;
    using band_type = InterTreeBand<T>;
    using band_set = std::set<band_type *>;
    OttIdSet getPhantomIds(const node_type * nd) const {
        OttIdSet r;
        const band_set & bs =  getBandsForNode(nd);
        for (const auto & b : bs) {
            const OttIdSet bo = b->getPhantomIds(nd);
            r.insert(begin(bo), end(bo));
        }
        return r;
    }
    const band_set & getBandsForNode(const node_type *n) const {
        const auto nit = node2Band.find(n);
        if (nit == node2Band.end()) {
            return emptySet;
        }
        return nit->second;
    }
    bool isInABand(const node_type *n) const {
        return contains(node2Band, n);
    }
    void updateToReflectResolution(node_type *oldAnc,
                                   node_type * newAnc,
                                   const std::set<node_type *> & movedTips,
                                   const PhyloStatement & ps);
    private:
    std::map<const band_type *, node_type *> band2Node;
    std::map<const node_type *, band_set > node2Band;
    const band_set emptySet;
};

template<typename T, typename U>
class FTree {
    public:
    using node_type = RootedTreeNode<T>;
    using GroupingConstraint = std::pair<node_type*, PhyloStatementSource>;
    using NdToConstrainedAt = std::map<node_type *, std::set<node_type *> >;
    FTree(std::size_t treeID,
          RootedForest<T, U> & theForest,
          std::map<long, node_type *> & ottIdToNodeRef)
        :treeId(treeID),
         root(nullptr),
         forest(theForest),
         ottIdToNodeMap(ottIdToNodeRef) {
    }
    // const methods:
    bool isInABand(const node_type *n) const {
        return bands.isInABand(n);
    }
    bool hasNodesExcludedFromIt(const node_type *n) const {
        return exclude.hasNodesExcludedFromIt(n);
    }
    const node_type * getRoot() const {
        return root;
    }
    bool ottIdIsExcludedFromRoot(long oid) const {
        return isExcludedFromRoot(ottIdToNodeMap.at(oid));
    }
    bool isExcludedFromRoot(const node_type *n) const {
        return exclude.isExcludedFrom(n, root);
    }
    // OTT Ids of nodes on the graph only....
    const OttIdSet getConnectedOttIds() const;
    // includes OTT Ids of nodes in includesConstraints
    const OttIdSet & getIncludedOttIds() {
        return getRoot()->getData().desIds;
    }
    bool ottIdIsConnected(long ottId) const {
        return contains(getConnectedOttIds(), ottId);
    }
    // non-const
    node_type * getRoot() {
        return root;
    }
    void mirrorPhyloStatement(const PhyloStatement & ps);
    void debugInvariantsCheckFT() const;
    void debugVerifyDesIdsAssumingDes(const OttIdSet &s, const RootedTreeNode<T> *nd) const;
    std::set<RootedTreeNode<T> *> ottIdSetToNodeSet(const OttIdSet &ottIdSet) const;
    private:
    void addExcludeStatement(long ottId, RootedTreeNode<T> *, const PhyloStatementSource &);
    void addIncludeGroupDisjointPhyloStatement(const PhyloStatement & ps) {
        addPhyloStatementAsChildOfRoot(ps);
    }
    void addIncludeStatement(long ottId, RootedTreeNode<T> *, const PhyloStatementSource &);
    RootedTreeNode<T> * addLeafNoDesUpdate(RootedTreeNode<T> * par, long ottId);
    void addPhyloStatementAsChildOfRoot(const PhyloStatement &);
    // this is greedy, we should be building separate FTree instances in many cases....
    OttIdSet addPhyloStatementAtNode(const PhyloStatement & ps, 
                                     node_type * includeGroupMRCA,
                                     const OttIdSet & attachedElsewhere);
    bool anyExcludedAtNode(const node_type *, const OttIdSet &) const ;
    bool anyPhantomNodesAtNode(const node_type *, const OttIdSet &) const ;
    bool anyIncludedAtNode(const node_type *, const OttIdSet &) const ;
    void createDeeperRoot();
    node_type * getMRCA(const OttIdSet &id);
    RootedTreeNode<T> * resolveToCreateCladeOfIncluded(RootedTreeNode<T> * par, const PhyloStatement & ps);
    
    friend class RootedForest<T, U>;
    friend class GreedyPhylogeneticForest<T, U>;
    FTree(const FTree &) = delete;
    FTree & operator=(const FTree &) = delete;
    // data members
    const std::size_t treeId; // key for this tree in forest - used for debugging
    node_type * root;
    //OttIdSet connectedIds;
    // from excludedNode to the nodes that it is excluded from...
    ExcludeConstraints<T> exclude;
    InterTreeBandBookkeeping<T> bands;
    //std::map<node_type *, std::list<PhyloStatementSource> > supportedBy; // only for non-roots
    RootedForest<T, U> & forest;
    std::map<long, node_type *> & ottIdToNodeMap;
};

template<typename T, typename U>
inline std::set<RootedTreeNode<T> *> FTree<T,U>::ottIdSetToNodeSet(const OttIdSet &ottIdSet) const {
    std::set<RootedTreeNode<T> *> ns;
    for (auto oid :ottIdSet) {
        ns.insert(ottIdToNodeMap.at(oid));
    }
    return ns;
}

template<typename T, typename U>
inline void FTree<T,U>::addExcludeStatement(long ottId,
                                            RootedTreeNode<T> * excludedFrom,
                                            const PhyloStatementSource &) {
    auto eNode = ottIdToNodeMap.at(ottId);
    exclude.addExcludeStatement(eNode, excludedFrom);
}

} // namespace otc
#endif