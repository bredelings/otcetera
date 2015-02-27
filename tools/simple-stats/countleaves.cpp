#include "otc/otcli.h"
#include "otc/tree_operations.h"
#pragma clang diagnostic ignored "-Wunused-variable"
using namespace otc;
typedef RootedTree<RTNodeNoData, RTreeNoData> Tree_t;

template<typename T>
bool writeNumLeaves(OTCLI & , std::unique_ptr<T> tree);

template<typename T>
bool writeNumLeaves(OTCLI & , std::unique_ptr<T> tree) {
	auto c = 0U;
	for (auto nd : ConstLeafIter<T>(*tree)) {
		c += 1;
	}
	std::cout << c << '\n';
	return true;
}

int main(int argc, char *argv[]) {
	OTCLI otCLI("otccountleaves",
				 "takes a filepath to a newick file and reports the number of leaves",
				 {"some.tre"});
	return treeProcessingMain<Tree_t>(otCLI, argc, argv, writeNumLeaves, nullptr, 1);
}
