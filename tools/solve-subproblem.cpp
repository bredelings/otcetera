#include <algorithm>
#include <set>
#include <list>
#include <iterator>

#include "otc/otcli.h"
#include "otc/tree_operations.h"
#include "otc/tree_iter.h"
using namespace otc;
using std::vector;
using std::unique_ptr;
using std::set;
using std::list;
using std::map;
using std::string;
using namespace otc;

typedef TreeMappedWithSplits Tree_t;

bool verbose = false;

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::set<T>& s)
{
    auto it = s.begin();
    o<<*it++;
    for(; it != s.end(); it++)
        o<<" "<<*it;
    return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::list<T>& s)
{
    auto it = s.begin();
    o<<*it++;
    for(; it != s.end(); it++)
        o<<" "<<*it;
    return o;
}

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& s)
{
    auto it = s.begin();
    o<<*it++;
    for(; it != s.end(); it++)
        o<<" "<<*it;
    return o;
}

/// Create a SORTED vector from a set
template <typename T>
vector<T> set_to_vector(const set<T>& s)
{
    vector<T> v;
    v.reserve(s.size());
    std::copy(s.begin(), s.end(), std::back_inserter(v));
    return v;
}

struct RSplit
{
    vector<int> in;
    vector<int> out;
    vector<int> all;
    RSplit() = default;
    RSplit(const set<int>& i, const set<int>& a)
        {
            in  = set_to_vector(i);
            all = set_to_vector(a);
            set_difference(begin(all), end(all), begin(in), end(in), std::inserter(out, out.end()));
            assert(in.size() + out.size() == all.size());
        }
};

std::ostream& operator<<(std::ostream& o, const RSplit& s)
{
    o<<s.in<<" | ";
    if (s.out.size() < 100)
        o<<s.out;
    else
    {
        auto it = s.out.begin();
        for(int i=0;i<100;i++)
            o<<*it++<<" ";
        o<<"...";
    }
    return o;
}

/// Merge components c1 and c2 and return the component name that survived
int merge_components(int c1, int c2, vector<int>& component, vector<list<int>>& elements)
{
    if (elements[c2].size() > elements[c1].size())
        std::swap(c1,c2);

    for(int i:elements[c2])
        component[i] = c1;

    elements[c1].splice(elements[c1].end(), elements[c2]);
  
    return c1;
}

bool empty_intersection(const set<int>& xs, const vector<int>& ys)
{
    for(int y: ys)
        if (xs.count(y))
            return false;
    return true;
}

vector<int> indices;

/// Construct a tree with all the splits mentioned, and return a null pointer if this is not possible
unique_ptr<Tree_t> BUILD(const vector<int>& tips, const vector<const RSplit*>& splits)
{
    std::unique_ptr<Tree_t> tree(new Tree_t());
    tree->createRoot();

    // 1. First handle trees of size 1 and 2
    if (tips.size() == 1)
    {
        tree->getRoot()->setOttId(*tips.begin());
        return tree;
    }
    else if (tips.size() == 2)
    {
        auto Node1a = tree->createChild(tree->getRoot());
        auto Node1b = tree->createChild(tree->getRoot());
        auto it = tips.begin();
        Node1a->setOttId(*it++);
        Node1b->setOttId(*it++);
        return tree;
    }

    // 2. Initialize the mapping from elements to components
    vector<int> component;       // element index  -> component
    vector<list<int>> elements;  // component -> element indices
    for(int i=0;i<tips.size();i++)
    {
        indices[tips[i]] = i;
        component.push_back(i);
        elements.push_back({i});
    }

    // 3. For each split, all the leaves in the include group must be in the same component
    for(const auto& split: splits)
    {
        int c1 = -1;
        for(int i: split->in)
        {
            int j = indices[i];
            int c2 = component[j];
            if (c1 != -1 and c1 != c2)
                merge_components(c1,c2,component,elements);
            c1 = component[j];
        }
    }

    // 4. If we can't subdivide the leaves in any way, then the splits are not consistent, so return failure
    if (elements[component[0]].size() == tips.size())
        return {};

    // 5. Make a vector of labels for the partition components
    vector<int> component_labels;                           // index -> component label
    vector<int> component_label_to_index(tips.size(),-1);   // component label -> index
    for(int c=0;c<tips.size();c++)
        if (c == component[c])
        {
            int index = component_labels.size();
            component_labels.push_back(c);
            component_label_to_index[c] = index;
        }

    
    // 6. Create the vector of tips in each connected component 
    vector<vector<int>> subtips(component_labels.size());
    for(int i=0;i<component_labels.size();i++)
    {
        vector<int>& s = subtips[i];
        int c = component_labels[i];
        for(int j: elements[c])
            s.push_back(tips[j]);
    }

    // 7. Determine the splits that are not satisfied yet and go into each component
    vector<vector<const RSplit*>> subsplits(component_labels.size());
    for(const auto& split: splits)
    {
        int first = indices[*split->in.begin()];
        assert(first >= 0);
        int c = component[first];

        // if none of the exclude group are in the component, then the split is satisfied by the top-level partition.
        bool satisfied = true;
        for(int x: split->out)
            if (indices[x] != -1 and component[indices[x]] == c)
            {
                satisfied = false;
                break;
            }
    
        if (not satisfied)
        {
            int i = component_label_to_index[c];
            subsplits[i].push_back(split);
        }
    }
  
    // 8. Clear our map from id -> index, for use by subproblems.
    for(int id: tips)
        indices[id] = -1;
  
    // 9. Recursively solve the sub-problems of the partition components
    for(int i=0;i<subtips.size();i++)
    {
        auto subtree = BUILD(subtips[i], subsplits[i]);
        if (not subtree) return {};

        addSubtree(tree->getRoot(), *subtree);
    }

    return tree;
}

unique_ptr<Tree_t> BUILD(const vector<int>& tips, const vector<RSplit>& splits)
{
    vector<const RSplit*> split_ptrs;
    for(const auto& split: splits)
        split_ptrs.push_back(&split);
    return BUILD(tips, split_ptrs);
}

/// Copy node names from taxonomy to tree based on ott ids, and copy the root name also
void add_names(unique_ptr<Tree_t>& tree, const unique_ptr<Tree_t>& taxonomy)
{
    clearAndfillDesIdSets(*tree);

    for(auto n1: iter_post(*tree))
        for(auto n2: iter_post(*taxonomy))
            if (n1->getData().desIds == n2->getData().desIds)
                n1->setName( n2->getName());
}

set<int> remap_ids(const set<long>& s1, const map<long,int>& id_map)
{
    set<int> s2;
    for(auto x: s1)
    {
        auto it = id_map.find(x);
        assert(it != id_map.end());
        s2.insert(it->second);
    }
    return s2;
}

/// Get the list of splits, and add them one at a time if they are consistent with previous splits
unique_ptr<Tree_t> combine(const vector<unique_ptr<Tree_t>>& trees)
{
    // 0. Standardize names to 0..n-1 for this subproblem
    const auto& taxonomy = trees.back();
    auto all_leaves = taxonomy->getRoot()->getData().desIds;

    // index -> id
    vector<long> ids;
    // id -> index
    map<long,int> id_map;
    for(long id: all_leaves)
    {
        int i = ids.size();
        id_map[id] = i;
        ids.push_back(id);

        assert(id_map[ids[i]] == i);
        assert(ids[id_map[id]] == id);
    }
    auto remap = [&id_map](const set<long>& ids) {return remap_ids(ids,id_map);};
    vector<int> all_leaves_indices;
    for(int i=0;i<all_leaves.size();i++)
        all_leaves_indices.push_back(i);

    indices.resize(all_leaves.size());
    for(auto& i: indices)
        i=-1;
  
    // 1. Find splits in order of input trees
    vector<RSplit> consistent;
    for(const auto& tree: trees)
    {
        auto root = tree->getRoot();
        const auto leafTaxa = root->getData().desIds;

        for(const auto& leaf: set_difference_as_set(leafTaxa, all_leaves))
            throw OTCError()<<"OTT Id "<<leaf<<" not in taxonomy!";
      
        const auto leafTaxaIndices = remap(leafTaxa);
        for(auto nd: iter_post_const(*tree))
        {
            const auto& descendants = remap(nd->getData().desIds);
            RSplit split{descendants, leafTaxaIndices};
            if (split.in.size()>1 and split.out.size())
            {
                consistent.push_back(split);
                auto result = BUILD(all_leaves_indices, consistent);
                if (not result)
                {
                    consistent.pop_back();
                    if (verbose and nd->hasOttId()) LOG(INFO)<<"Reject: ott"<<nd->getOttId()<<"\n";
                }
                else
                {
                    if (verbose and nd->hasOttId()) LOG(INFO)<<"Keep: ott"<<nd->getOttId()<<"\n";
                }
            }
        }
    }

    // 2. Construct final tree and add names
    auto tree = BUILD(all_leaves_indices, consistent);
    for(auto nd: iter_pre(*tree))
        if (nd->isTip())
        {
            int index = nd->getOttId();
            nd->setOttId(ids[index]);
        }

    add_names(tree, taxonomy);
    return tree;
}

bool get_bool(const string& arg, const string& context="")
{
    if (arg == "true" or arg == "yes" or arg == "True" or arg == "Yes")
        return true;
    else if (arg == "false" or arg == "no" or arg == "False" or arg == "No")
        return false;
    else
        throw OTCError()<<context<<"'"<<arg<<"' is not a recognized boolean value.";
}

bool handleRequireOttIds(OTCLI & otCLI, const std::string & arg)
{
    otCLI.getParsingRules().setOttIds = get_bool(arg,"-o: ");
    return true;
}

bool handlePruneUnrecognizedTips(OTCLI & otCLI, const std::string & arg)
{
    otCLI.getParsingRules().pruneUnrecognizedInputTips = get_bool(arg,"-p: ");
    return true;
}

bool synthesize_taxonomy = false;

bool handleSynthesizeTaxonomy(OTCLI &, const std::string &arg)
{
    if (arg.size())
        throw OTCError()<<"-T does not take an argument.";
    synthesize_taxonomy = true;
    return true;
}

bool cladeTips = true;

bool handleCladeTips(OTCLI &, const std::string & arg)
{
    cladeTips = get_bool(arg,"-i: ");
    return true;
}

bool writeStandardized = false;

bool handleStandardize(OTCLI& otCLI, const std::string & arg)
{
    if (arg.size())
        throw OTCError()<<"-S does not take an argument.";
    writeStandardized = true;
    otCLI.getParsingRules().setOttIds = false;
    return true;
}

string rootName = "";

bool handleRootName(OTCLI& otCLI, const std::string & arg)
{
    rootName = arg;
    return true;
}


/// Create an unresolved taxonomy out of all the input trees.
unique_ptr<Tree_t> make_unresolved_tree(const vector<unique_ptr<Tree_t>>& trees, bool use_ids)
{
    std::unique_ptr<Tree_t> tree(new Tree_t());
    tree->createRoot();

    if (use_ids)
    {
        map<long,string> names;
        for(const auto& tree: trees)
            for(auto nd: iter_pre_const(*tree))
                if (nd->isTip())
                {
                    long id = nd->getOttId();
                    auto it = names.find(id);
                    if (it == names.end())
                        names[id] = nd->getName();
                }
  
        for(const auto& n: names)
        {
            auto node = tree->createChild(tree->getRoot());
            node->setOttId(n.first);
            node->setName(n.second);
        }
        clearAndfillDesIdSets(*tree);
    }
    else
    {
        set<string> names;
        for(const auto& tree: trees)
            for(auto nd: iter_pre_const(*tree))
                if (nd->isTip())
                    names.insert(nd->getName());

        for(const auto& n: names)
        {
            auto node = tree->createChild(tree->getRoot());
            node->setName(n);
        }
    }

    return tree;
}

string newick(const Tree_t &t)
{
    std::ostringstream s;
    writeTreeAsNewick(s, t);
    return s.str();
}

/// Create a mapping from name -> id
map<string, long> createIdsFromNames(const Tree_t& taxonomy)
{
    long id = 1;
    map<string,long> name_to_id;
    for(auto nd: iter_post_const(taxonomy))
        if (nd->getName().size())
        {
            string name = nd->getName();
            auto it = name_to_id.find(name);
            if (it != name_to_id.end())
                throw OTCError()<<"Tip label '"<<name<<"' occurs twice in taxonomy!";
            name_to_id[name] = id++;
        }
        else if (nd->isTip())
            throw OTCError()<<"Taxonomy tip has no label!";

    return name_to_id;
}  

/// Set ids on the tree based on the name
void setIdsFromNames(Tree_t& tree, const map<string,long>& name_to_id)
{
    for(auto nd: iter_post(tree))
        if (nd->getName().size())
        {
            string name = nd->getName();
            auto it = name_to_id.find(name);
            if (it == name_to_id.end())
                throw OTCError()<<"Can't find label '"<<name<<"' in taxonomy!";
            auto id = it->second;
            nd->setOttId(id);
            tree.getData().ottIdToNode[id] = nd;
        }
        else if (nd->isTip())
            throw OTCError()<<"Tree tip has no label!";
  
    clearAndfillDesIdSets(tree);
}

string addOttId(const string s, long id)
{
    string tag = "ott" + std::to_string(id);
    if (not s.size())
        return tag;
    else
        return s + " " + tag;
}

void relabelWithOttId(Tree_t& T)
{
    for(auto nd: iter_pre(T))
        if (nd->hasOttId())
            nd->setName(addOttId(nd->getName(),nd->getOttId()));
}

int main(int argc, char *argv[]) {
    OTCLI otCLI("otc-solve-subproblem",
                "Takes a series of tree files.\n"
                "Files are concatenated and the combined list treated as a single subproblem.\n"
                "Trees should occur in order of priority, with the taxonomy last.\n",
                "subproblem.tre");

    otCLI.addFlag('o',
                  "Require OTT ids.  Defaults to true",
                  handleRequireOttIds,
                  true);

    otCLI.addFlag('p',
                  "Prune unrecognized tips.  Defaults to false",
                  handlePruneUnrecognizedTips,
                  true);

    otCLI.addFlag('i',
                  "Tips may be internal nodes on the taxnomy.  Defaults to true",
                  handleCladeTips,
                  true);

    otCLI.addFlag('n',
                  "Rename the root to this name",
                  handleRootName,
                  true);

    otCLI.addFlag('T',
                  "Synthesize an unresolved taxonomy from all mentioned tips.  Defaults to false",
                  handleSynthesizeTaxonomy,
                  false);

    otCLI.addFlag('S',
                  "Write out a standardized subproblem and exit",
                  handleStandardize,
                  false);

    vector<unique_ptr<Tree_t>> trees;
    auto get = [&trees](OTCLI &, unique_ptr<Tree_t> nt) {trees.push_back(std::move(nt)); return true;};

    if (argc < 2)
        throw OTCError("No subproblem provided!");

    // I think multiple subproblem files are essentially concatenated.
    // Is it possible to read a single subproblem from cin?
    if (treeProcessingMain<Tree_t>(otCLI, argc, argv, get, nullptr, 1))
        std::exit(1);

    verbose = otCLI.verbose;

    if (trees.empty())
        throw OTCError("No trees loaded!");

    bool setOttIds = otCLI.getParsingRules().setOttIds;
    if (synthesize_taxonomy)
    {
        trees.push_back(make_unresolved_tree(trees,setOttIds));
        LOG(DEBUG)<<"taxonomy = "<<newick(*trees.back())<<"\n";
    }

    // Add fake Ott Ids to tips and compute desIds
    if (not setOttIds)
    {
        auto name_to_id = createIdsFromNames(*trees.back());
        for(auto& tree: trees)
            setIdsFromNames(*tree, name_to_id);
    }

    if (writeStandardized)
    {
        for(const auto& tree: trees)
        {
            relabelWithOttId(*tree);
            std::cout<<newick(*tree)<<"\n";
        }	
        exit(0);
    }
    
    // Check if trees are mapping to non-terminal taxa, and either fix the situation or die.
    for(int i=0;i<trees.size()-1;i++)
        if (cladeTips)
            expandOTTInternalsWhichAreLeaves(*trees[i], *trees.back());
        else
            requireTipsToBeMappedToTerminalTaxa(*trees[i], *trees.back());

    auto tree = combine(trees);
    
    if (not rootName.empty())
        tree->getRoot()->setName(rootName);

    writeTreeAsNewick(std::cout, *tree);

    std::cout<<"\n";
}
