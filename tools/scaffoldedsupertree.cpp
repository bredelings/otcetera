#include "otc/embedding_cli.h"
using namespace otc;

enum SuperTreeDOTStep {
    BEFORE_ND_WO_TAXO,
    BEFORE_ND_W_TAXO,
    AFTER_ND_WO_TAXO,
    AFTER_ND_W_TAXO
};

class ScaffoldedSupertree : public EmbeddingCLI {
    public:
    bool doReportAllContested;
    bool doConstructSupertree;
    std::list<long> idsListToReportOn;
    std::list<long> idListForDotExport;
    int currDotFileIndex;
    bool emitScaffoldDotFiles;
    long verboseLoggingTarget;
    long ancToBeConsidered;

    virtual ~ScaffoldedSupertree(){}
    ScaffoldedSupertree()
        :EmbeddingCLI(),
         doReportAllContested(false),
         doConstructSupertree(false),
         currDotFileIndex(0),
         emitScaffoldDotFiles(false),
         ancToBeConsidered(-1) {
    }

    void writeEmbeddingDOT(SuperTreeDOTStep sts, const NodeWithSplits * nd, const NodeWithSplits * actionNd) {
        std::string fn = "ScaffSuperTree_num";
        fn += std::to_string(currDotFileIndex++);
        fn += "_";
        const bool includeLastTree = (sts == BEFORE_ND_W_TAXO || sts == AFTER_ND_W_TAXO);
        if (sts == BEFORE_ND_WO_TAXO || sts == BEFORE_ND_W_TAXO) {
            fn += "Nd";
            fn += std::to_string(nd->getOttId());
            fn += "Before";
        } else if (sts == AFTER_ND_WO_TAXO || sts == AFTER_ND_W_TAXO) {
            fn += "Nd";
            fn += std::to_string(nd->getOttId());
            fn += "After";
        }
        if (actionNd != nullptr) {
            fn += "Nd";
            fn += std::to_string(actionNd->getOttId());
        }
        if (includeLastTree) {
            fn += "WTax";
        }
        fn += ".dot";
        std::ofstream out;
        out.open(fn);
        const auto & thr = _getEmbeddingForNode(nd);
        writeDOTExport(out, thr, nd, treePtrByIndex, true, includeLastTree);
    }

    void writeNumberedDOT(NodeWithSplits * nd, bool entireSubtree, bool includeLastTree) {
        std::string fn = "ScaffSuperTree" + std::to_string(currDotFileIndex++) + ".dot";
        LOG(DEBUG) << "writing DOT file \"" << fn << "\"";
        std::ofstream out;
        out.open(fn);
        const auto & thr = _getEmbeddingForNode(nd);
        writeDOTExport(out, thr, nd, treePtrByIndex, entireSubtree, includeLastTree);
        LOG(DEBUG) << "finished DOT file \"" << fn << "\"";
    }

    void resolveOrCollapse(NodeWithSplits * scaffoldNd, SupertreeContextWithSplits & sc) {
        assert(!scaffoldNd->isTip());
        auto & thr = _getEmbeddingForNode(scaffoldNd);
        LOG(INFO) << "  outdegree = " << scaffoldNd->getOutDegree() << " numLoopTrees = " << thr.getNumLoopTrees() << " numLoops = " << thr.getTotalNumLoops();
        if (thr.isContested()) {
            LOG(INFO) << "    Contested";
            if (thr.highRankingTreesPreserveMonophyly(sc.numTrees)) {
                thr.resolveGivenContestedMonophyly(*scaffoldNd, sc);
            } else {
                thr.constructPhyloGraphAndCollapseIfNecessary(*scaffoldNd, sc);
            }
        } else {
            LOG(INFO) << "    Uncontested";
            thr.resolveGivenUncontestedMonophyly(*scaffoldNd, sc);
        }
    }

    void constructSupertree(OTCLI &otCLI) {
        TreeMappedWithSplits * tax = taxonomy.get();
        SupertreeContextWithSplits sc{treePtrByIndex, scaffoldNdToNodeEmbedding, *tax};
        if (debuggingOutput) {
            if (emitScaffoldDotFiles) {
                writeEmbeddingDOT(BEFORE_ND_W_TAXO, taxonomy->getRoot(), nullptr);
                writeEmbeddingDOT(BEFORE_ND_WO_TAXO, taxonomy->getRoot(), nullptr);
            } else {
                LOG(DEBUG) << "Beginning construction of the supertree from the embedded tree.";
            }
        }
        std::list<NodeWithSplits * > postOrder;
        for (auto nd : iter_post(*taxonomy)) {
            if (nd->isTip()) {
                assert(nd->hasOttId());
                // this is only needed for monotypic cases in which a tip node
                //  may have multiple OTT Ids in its desIds set
                _getEmbeddingForNode(nd).setOttIdForExitEmbeddings(nd,
                                                                   nd->getOttId(),
                                                                   scaffoldNdToNodeEmbedding);
            } else {
                postOrder.push_back(nd);
            }
        }
        NodeWithSplits * ancToAnalyze = nullptr;
        if (ancToBeConsidered >= 0) {
            ancToAnalyze = taxonomy->getData().ottIdToNode.at(ancToBeConsidered);
        }
        for (auto nd : postOrder) {
            if (nd->getOttId() == verboseLoggingTarget) {
                otCLI.turnOnVerboseMode();
            }
            if (ancToAnalyze != nullptr && (ancToAnalyze != nd && !isAncestorDesNoIter(ancToAnalyze, nd))) {
                continue;
            }
            auto p = nd->getParent();
            assert(!nd->isTip());
            if (debuggingOutput) {
                if (emitScaffoldDotFiles) {
                    writeEmbeddingDOT(BEFORE_ND_W_TAXO, nd, nd);
                    writeEmbeddingDOT(BEFORE_ND_WO_TAXO, nd, nd);
                }
            }
            LOG(INFO) << "Calling resolveOrCollapse for OTT" << nd->getOttId() << " " << nd->getName();
            resolveOrCollapse(nd, sc);
            if (debuggingOutput) {
                if (emitScaffoldDotFiles) {
                    writeEmbeddingDOT(AFTER_ND_W_TAXO, nd, nd);
                    writeEmbeddingDOT(AFTER_ND_WO_TAXO, nd, nd);
                    writeEmbeddingDOT(AFTER_ND_W_TAXO, taxonomy->getRoot(), nd);
                    writeEmbeddingDOT(AFTER_ND_WO_TAXO, taxonomy->getRoot(), nd);
                } else {
                    LOG(DEBUG) << "Completed resolveOrCollapse call for OTT" << nd->getOttId();
                }
            }
            if (p != nullptr) {
                checkAllNodePointersIter(*p);
            }
            bool before = true;
            for (auto u : postOrder) {
                if (u == nd) {
                    before = false;
                } else if ((!before) && u->isTip()) {
                    LOG(ERROR) << "Node for OTT" << u->getOttId() << " has become a tip after processing OTT" << nd->getOttId();
                    assert(false);
                    throw OTCError("false assertion disabled.");
                }
            }
            if (nd->getOttId() == verboseLoggingTarget) {
                otCLI.turnOffVerboseMode();
            }
            
        }
    }

    void reportAllConflicting(std::ostream & out, bool verbose) {
        std::map<std::size_t, unsigned long> nodeMappingDegree;
        std::map<std::size_t, unsigned long> passThroughDegree;
        std::map<std::size_t, unsigned long> loopDegree;
        unsigned long totalContested = 0;
        unsigned long redundContested = 0;
        unsigned long totalNumNodes = 0;
        for (auto nd : iter_node_internal(*taxonomy)) {
            const auto & thr = _getEmbeddingForNode(nd);
            nodeMappingDegree[thr.getTotalNumNodeMappings()] += 1;
            passThroughDegree[thr.getTotalNumEdgeBelowTraversals()] += 1;
            loopDegree[thr.getTotalNumLoops()] += 1;
            totalNumNodes += 1;
            std::vector<NodeWithSplits *> aliasedBy = getNodesAliasedBy(nd, *taxonomy);
            if (thr.reportIfContested(out, nd, treePtrByIndex, aliasedBy, verbose)) {
                totalContested += 1;
                if (nd->getOutDegree() == 1) {
                    redundContested += 1;
                }
            }
        }
        unsigned long m = std::max(loopDegree.rbegin()->first, passThroughDegree.rbegin()->first);
        m = std::max(m, nodeMappingDegree.rbegin()->first);
        out << "Degree\tNodeMaps\tEdgeMaps\tLoops\n";
        for (unsigned long i = 0 ; i <= m; ++i) {
            out << i << '\t' << nodeMappingDegree[i]<< '\t' << passThroughDegree[i] << '\t' << loopDegree[i]<< '\n';
        }
        out << totalNumNodes << " internals\n" << totalContested << " contested\n" << (totalNumNodes - totalContested) << " uncontested\n";
        out << redundContested << " monotypic contested\n";
    }

    bool summarize(OTCLI &otCLI) override {
        if (doConstructSupertree ) {
            cloneTaxonomyAsASourceTree();
            constructSupertree(otCLI);
            writeTreeAsNewick(otCLI.out, *taxonomy);
            otCLI.out << '\n';
        }
        std::ostream & out{otCLI.out};
        assert (taxonomy != nullptr);
        if (doReportAllContested) {
            reportAllConflicting(out, otCLI.verbose);
        } else {
            for (auto tr : idsListToReportOn) {
                auto nd = taxonomy->getData().getNodeForOttId(tr);
                if (nd == nullptr) {
                    throw OTCError(std::string("Unrecognized OTT ID in list of OTT IDs to report on: ") + std::to_string(tr));
                }
                const auto & thr = _getEmbeddingForNode(nd);
                std::vector<NodeWithSplits *> aliasedBy = getNodesAliasedBy(nd, *taxonomy);
                thr.reportIfContested(out, nd, treePtrByIndex, aliasedBy, otCLI.verbose);
            }
        }
        for (auto tr : idListForDotExport) {
            auto nd = taxonomy->getData().getNodeForOttId(tr);
            if (nd == nullptr) {
                throw OTCError(std::string("Unrecognized OTT ID in list of OTT IDs to export to DOT: ") + std::to_string(tr));
            }
            for (auto n : iter_pre_n_const(nd)) {
                const auto & thr = _getEmbeddingForNode(n);
                writeDOTExport(out, thr, n, treePtrByIndex, false, false);
            }
        }
        return true;
    }
};

bool handleReportAllFlag(OTCLI & otCLI, const std::string &);
bool handleReportOnNodesFlag(OTCLI & otCLI, const std::string &);
bool handleDotNodesFlag(OTCLI & otCLI, const std::string &narg);
bool handleSuperTreeFlag(OTCLI & otCLI, const std::string &narg);

bool handleReportAllFlag(OTCLI & otCLI, const std::string &) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    assert(proc != nullptr);
    proc->doReportAllContested = true;
    return true;
}
bool handleSuperTreeFlag(OTCLI & otCLI, const std::string &) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    assert(proc != nullptr);
    proc->doConstructSupertree = true;
    return true;
}

bool handleReportOnNodesFlag(OTCLI & otCLI, const std::string &narg) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    assert(proc != nullptr);
    if (narg.empty()) {
        throw OTCError("Expecting a list of IDs after the -b argument.");
    }
    auto rs = split_string(narg, ',');
    for (auto word : rs) {
        auto ottId = stringToOttID(word);
        if (ottId < 0) {
            throw OTCError(std::string("Expecting a list of IDs after the -b argument. Offending word: ") + word);
        }
        proc->idsListToReportOn.push_back(ottId);
    }
    return true;
}

bool handleOttForestDOTFlag(OTCLI & otCLI, const std::string &narg);
bool handleAncFlag(OTCLI & otCLI, const std::string &narg);
bool handleOttVerboseLogTargetFlag(OTCLI & otCLI, const std::string &narg);
bool handleOttScaffoldDOTFlag(OTCLI & otCLI, const std::string &);
bool handleDotNodesFlag(OTCLI & otCLI, const std::string &narg);

bool handleOttForestDOTFlag(OTCLI & , const std::string &narg) {
    long conv = -1;
    if (!char_ptr_to_long(narg.c_str(), &conv) || conv < 0) {
        throw OTCError(std::string("Expecting a positive number as an ott ID after -z flag. Offending word: ") + narg);
    }
    ottIDBeingDebugged = conv;
    return true;
}

bool handleAncFlag(OTCLI & otCLI, const std::string &narg) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    long conv = -1;
    if (!char_ptr_to_long(narg.c_str(), &conv) || conv < 0) {
        throw OTCError(std::string("Expecting a positive number as an ott ID after -z flag. Offending word: ") + narg);
    }
    proc->ancToBeConsidered = conv;
    return true;
}

bool handleOttVerboseLogTargetFlag(OTCLI & otCLI, const std::string &narg) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    long conv = -1;
    if (!char_ptr_to_long(narg.c_str(), &conv) || conv < 0) {
        throw OTCError(std::string("Expecting a positive number as an ott ID after -z flag. Offending word: ") + narg);
    }
    proc->verboseLoggingTarget = conv;
    return true;
}


bool handleOttScaffoldDOTFlag(OTCLI & otCLI, const std::string &) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    assert(proc != nullptr);
    proc->emitScaffoldDotFiles = true;
    return true;
}

bool handleDotNodesFlag(OTCLI & otCLI, const std::string &narg) {
    ScaffoldedSupertree * proc = static_cast<ScaffoldedSupertree *>(otCLI.blob);
    assert(proc != nullptr);
    if (narg.empty()) {
        throw OTCError("Expecting a list of IDs after the -d argument.");
    }
    auto rs = split_string(narg, ',');
    for (auto word : rs) {
        auto ottId = ottIDFromName(word);
        if (ottId < 0) {
            throw OTCError(std::string("Expecting a list of IDs after the -d argument. Offending word: ") + word);
        }
        proc->idListForDotExport.push_back(ottId);
    }
    return true;
}


int main(int argc, char *argv[]) {
    OTCLI otCLI("otc-scaffolded-supertree",
                "takes at least 2 newick file paths: a full taxonomy tree, and some number of input trees. Crashes or emits bogus output",
                "taxonomy.tre inp1.tre inp2.tre");
    ScaffoldedSupertree proc;
    otCLI.addFlag('a',
                  "Write a report of all contested nodes",
                  handleReportAllFlag,
                  false);
    otCLI.addFlag('s',
                  "Compute a supertree",
                  handleSuperTreeFlag,
                  false);
    otCLI.addFlag('b',
                  "ARG should be a list of OTT IDs. A status report will be generated for those nodes",
                  handleReportOnNodesFlag,
                  true);
    otCLI.addFlag('d',
                  "ARG should be a list of OTT IDs. A DOT file of the nodes will be generated ",
                  handleDotNodesFlag,
                  true);
    otCLI.addFlag('z',
                  "ARG should be an OTT ID. A series DOT files will be generated for the forest created during the resolution of this OTT ID ",
                  handleOttForestDOTFlag,
                  true);
    otCLI.addFlag('c',
                  "ARG should be an OTT ID. Only descendants of this OTT ID will be processed during the supetree construction. This is intended to make debugging of large trees faster - the final results are not reliable",
                  handleAncFlag,
                  true);
    otCLI.addFlag('l',
                  "ARG should be an OTT ID. verbose logging mode will be enabled when solving the subproblem for this node",
                  handleOttVerboseLogTargetFlag,
                  true);
    otCLI.addFlag('y',
                  "requests DOT export of the embedded tree during the supertree operation - only for use on small examples!",
                  handleOttScaffoldDOTFlag,
                  false);
    return taxDependentTreeProcessingMain(otCLI, argc, argv, proc, 2, true);
}

