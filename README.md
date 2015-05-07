# otcetera - phylogenetic file format parser in C++ 
[![Build Status](https://secure.travis-ci.org/OpenTreeOfLife/otcetera.png)](http://travis-ci.org/OpenTreeOfLife/otcetera)

otcetera owes a lot of code and ideas to Paul Lewis' Nexus Class Library.
  See http://hydrodictyon.eeb.uconn.edu/ncl/ and 
  https://github.com/mtholder/ncl

It also uses easyloggingpp which is distributed under an MIT License. See
  http://github.com/easylogging/ for info on that project. The file from
  that project is otc/easylogging++.h

Some set comparisons (in util.h) were based on
   http://stackoverflow.com/posts/1964252/revisions
by http://stackoverflow.com/users/127669/graphics-noob

# Installation

## prerequisites

### rapidjson
To facilitate parsing of NexSON, this version of requires rapidjson.
Download it from https://github.com/miloyip/rapidjson

**Note**: you'll need to put the full path to the `rapidjson/include` subdir 
after a "-I" flag in either the CPPFLAGS variable or the CXXFLAGS variable
when you run configure for otcetera



### autotools
You also need the a fairly recent version of the whole autotools stack
including libtool. MTH had problems with automake 1.10. If you can't install
these with something like apt, then you can grab the sources. The following
worked for MTH on Mac on 28-Feb-2015:

    wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
    tar xfvz autoconf-2.69.tar.gz 
    cd autoconf-2.69
    ./configure 
    make
    sudo make install
    cd ..
    wget http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz
    tar xfvz automake-1.15.tar.gz 
    cd automake-1.15
    ./configure 
    make
    sudo make install


## configuration + building

To run the whole autoreconf stuff in a manner that will add missing bits as needed,
run:

    $ sh bootstrap.sh

Then to configure and build with clang use:

    $ mkdir buildclang
    $ cd buildclang
    $ bash ../reconf-clang.sh
    $ make
    $ make check
    $ make install
    $ make installcheck

To use g++, substitute `reconf-gcc.sh` for `reconf-clang.sh` in that work flow.

Python 2 (recent enough to have the subprocess module as part of the standard lib)
is required for the `make check` operation to succeed.

# Documentation
A LaTeX documentation file is [./doc/summarizing-taxonomy-plus-trees.tex](./doc/summarizing-taxonomy-plus-trees.tex)
periodically, that is compiled and posted.
The currently URL for that compiled documentation is http://phylo.bio.ku.edu/ot/summarizing-taxonomy-plus-trees.pdf

# Usage
See the [supertree/README.md](./supertree/README.md) for instructions on using
`otcetera` to build a supertree (work in progress).

## Common command line flags
The tools use the same (OTCLI) class to process command line arguments. 
This provides the following command line flags:
  * `-h` for help
  * `-fFILE` to treat every line of FILE as if it were a command line argument
      (useful for processing hundreds of filenames)
  * `-v` for verbose output
  * `-q` for quieter than normal output
  * `-t` for trace level (extremely verbose) output

Unless otherwise stated:

  1. the command line tools that need a tree take a filepath 
to a newick tree file. The numeric suffix of each label in the tree is taken to
be the OTT id. This accommodates the name munging that some of the open tree of
life tools perform on taxonomic names with special characters (because only the
OTT id is used to associate labels in different trees)
  2. a full supertree tree and taxonomy tree have the same leaf set in terms of OTT ids


## Tools for checking a supertree against inputs

### Checking for unnamed nodes in a full tree that have no tree supporting them

    otc-find-unsupported-nodes taxonomy.tre synth.tre inp1.tre inp2.tre ...

will report any nodes in `synth.tre` that are not named and which do not have
any [ITEB support](https://github.com/OpenTreeOfLife/treemachine/blob/nonsense-1/nonsense/iteb_support_theorem.md)

The taxonomy is just used for the ottID validation (on the assumption that 
the nodes supported by the taxonomy and the the otcchecktaxonomicnodes tool
can help identify problems with those nodes).

### Checking for incorrect internal labels in a full tree

    otc-check-taxonomic-nodes synth.tre taxonomy.tre

will check every labelled internal node is correctly labelled. To do this, it 
verifies that the set of OTT ids associated with tips that descend from the 
node is identical to the set of OTT ids associated with terminal taxa below
the corresponding node in the taxonomic tree.

A brief report will be issued for every problematic labeling. 
`otc-taxon-conflict-report` takes at least 2 newick file paths: a full tree, and some number of input trees.
It will write a summary of the difference in taxonomic inclusion for nodes that are in conflict:

    otc-taxon-conflict-report taxonomy.tre inp1.tre inp2.tre


### Checking for additional splits that could be added

    otc-find-resolution taxonomy.tre synth.tre tree1.tre tree2.tre ...

will look for groups in the input trees (`tree1.tre`, `tree2.tre`...) which could
resolve polytomies in `synth.tre`.  `taxonomy.tre` is used for label validation
and expanding any tips in input trees that are mapped to non-terminal taxa.

## Tools used in the supertree pipeline
### expanding tips mapped to higher taxa and pruning the taxonomy
`otc-nonterminals-to-exemplars` takes an -e flag specifying an export diretory and at least 2 newick file paths: a full taxonomy tree some number of input trees.
Any tip in non-taxonomic input that is mapped to non-terminal taoxn will be remapped such
that the parent of the non-terminal tip will hold all of the expanded exemplars.
The exemplars will be the union of tips that (a) occur below this non-terminal taxon in the taxonomy
and (b) occur, or are used as an exemplar, in another input tree.
The modified version of each input will be written in the export directory.
Trees with no non-terminal tips should be unaltered.
The taxonomy written out will be the taxonomy restricted to the set of leaves that are leaves of the exported trees:

    otc-nonterminals-to-exemplars -estep_5 taxonomy.tre inp1.tre inp2.tre ...

This is intended to perform steps 2.5 and 2.6 of the supertree pipeline mentioned in the `doc` subdirectory.

### pruning a taxonomy

    otc-prune-taxonomy taxonomy.tre inp1.tre inp2.tre ...

will write (to stdout) a newick version of the taxonomy that has been pruned to 
not include subtrees that do not include any of the tips of the input trees.  See
[supertree/step-2-pruned-taxonomy/README.md](./supertree/step-2-pruned-taxonomy/README.md)
for a more precise description of the pruning rules. This is intended to be used in the
[ranked tree supertree pipeline](./supertree/README.md),

### decomposing set of  trees into subproblems based on uncontested taxa

    otc-uncontested-decompose -eEXPORT taxonomy.tre -ftree-list.txt

will create subproblems in the (existing) subdirectory EXPORT using the taxonomy.tre
as the taxonomy and every tree listed in tree-list.txt. (each line of that file)
should be an input tree filepath. Each output will have:
  * a name that corresponds to the OTT taxon,
  * the trees pruned down for each subproblem (in the) same order as the trees were
      provided in the invocation, and
  * a corresponding ott###-tree-names.txt file that list the input filenames for each 
    tree (or "TAXONOMY" for taxonomy, which will always be the last tree).

*NOTE*: phylogenetic tips mapped to internal labels in the taxonomy will be pruned if 
   the taxon is contested. This is probably not what one usually wants to do...

### supertree using the decomposition and a greedy subproblem solver
`otc-scaffolded-supertree` is incomplete. If completed it will produces a supertree
of the its inputs.

## Miscellaneous tree manipulations and tree statistics

### Calculating stats for the subproblems
This works on the outputs of `otc-uncontested-decompose`. Running:

    otc-subproblem-stats *.tre > stats.tsv

Will create a tab-separated file of stats for the subproblems.
As of 5, May 2015, the columns of the report are:
  *  Subproblem name
  * InSp = # of informative (nontrivial) splits
  * LSS = size of the leaf label set
  * ILSS = size of the set of labels included in at least one "ingroup"
  * NT = The number of trees.
  * TreeSummaryName = tree index or summary name where the summary name can be Phylo-only or Total. 
  "Total" summarizes info all trees in the file (including the taxonomy).
  "Phylo-only" former summarizes all of the phylogenetic inputs.

Use the `-h` option to see an explanation of the columns if they differ from this list.


### getting the full distribution of out degree counts for a tree

    otc-degree-distribution sometree.tre

will write out a tab-separated pair of columns of "out degree" and "count" that
shows how many nodes in the tree tree have each outdegree (0 are leaves. 1 are
redundant nodes. 2 are fully resolved internals...)

### counting the number of polytomies in a tree

    otc-polytomy-count sometree.tre

will write out the number of nodes with out degree greater than 2 to stdout. This
is just a summary of the info reported by `otcdegreedistribution`.

**Untested**

### counting the number of leaves in a tree
`otc-count-leaves` takes a filepath to a newick file and reports the number of leaves:

    otc-count-leaves sometree.tre

### Detecting contested taxa

`otc-detect-contested` takes at least 2 newick file paths: a full taxonomy tree, and some number of input trees. 
It will print out the OTT IDs of clades in the taxonomy whose monophyly is questioned by at least one input:

    otc-detect-contested taxonomy.tre inp1.tre inp2.tre

### Get an induced subtree

`otc-induced-subtree` takes at least 2 newick file paths: a full tree, and some number of input trees. 
It will print a newick representation of the topology of the first tree if it is pruned down to the leafset of the inputs (without removing internal nodes):

    otc-induced-subtree taxonomy.tre inp1.tre

**Untested**

### Extract a subtree from a larger tree
`otc-prune-to-subtree`: Reads a large tree and takes a set of OTT Ids.
It finds the MRCA of the OTT Ids, and writes the subtree for that MRCA as newick.
The flag preceding the comma-separated list of IDs indicates whether the user
want the subtree for the MRCA node (`-n` flag), its parent(`-p` flag), each
of its children (`-c` flag and writing one line per child), or each
of its siblings (`-s` flag and writing one line per sib):

    otc-prune-to-subtree -p5315,3512 some.tre
    otc-prune-to-subtree -n5315,3512 some.tre
    otc-prune-to-subtree -c5315,3512 some.tre
    otc-prune-to-subtree -s5315,3512 some.tre
    

**Untested**

### Find distance between a supertree and the input trees
`otc-disance` takes at least 2 newick file paths: a supertree, and some number of input trees. 
It will print the Robinson-Foulds symmetric difference between the induced tree from the full tree to each 
input tree (one RF distance per line), or the number of groupings in each input tree that are 
either displayed or not displayed by the supertree

    otc-distance -r taxonomy.tre inp1.tre inp2.tre

Note the `otc-missing-splits` script reports just the splits in the induced tree that are
missing from the subsequent trees.
Comparing this number to the RF would reveal the number of groupings that are missing from the induced
tree but present in a subsequent tree.
Thus, one can calculate "missing" and "extra" grouping counts from the output of both tools.

**Untested**

### Suppress nodes of outdegree=1
`otc-suppress-monotypic` takes a filepath to a newick file and writes a newick 
without any nodes that have just one child:

    otc-suppress-monotypic taxonomy.tre

### debugging otcetera handling of trees

    otc-assert-invariants sometree.tre

will parse a tree and run through lots of traversals, asserting various
invariants. Should exit silently if there are no bugs.

# Testing
`otcetera` is still very much under development. You can trigger the running of the
tests by:

    $ make 
    $ make check

(currently there are no tests in the `make installcheck` target).
The data for running these tests is in the `data` subdirectory (but the tests
are supposed to know how to find that data, so users do not need to know the location).

## unit tests

Some of the operations have unit-tests. These tests are found in the `test` subdir.
Successful execution of these tests results in a row of periods (one per test) appearing
when the make check enters the test directory.

## tools tests
Some of the executables in the `tools` subdirectory have tests. These
are executed as a part of the normal `make check` target.
The output of the tool can be check using text comparison or tree comparisons
  (to handle cases in which branch rotation might result in multiple valid outputs
  of the same operation).
Some of the tests just check the exit code.

The syntax used to describe a new test is described in [../expected/README.md](../expected/README.md)
and the directories that describe the expected behavior are in the `expected` subdirectory.

## ACKNOWLEDGEMENTS
See comments above about usage of [easyloggingpp](https://github.com/easylogging/)
and [rapidjson](https://github.com/miloyip/rapidjson)

To acknowledge the contributions of the NCL code and ideas, a snapshot of the 
NCL credits taken from the version of NCL used to jump start otcetera is:

As of March 09, 2012, NCL is available under a Simplified BSD license (see
BSDLicense.txt) in addition to the GPL license.

NCL AUTHORS -- the author of the NEXUS Class Library (NCL) version 2.0 is

  Paul O. Lewis, Ph.D.
  Department of Ecology and Evolutionary Biology
  The University of Connecticut
  75 North Eagleville Road, Unit 3043
  Storrs, CT 06269-3043
  U.S.A.

  WWW: http://lewis.eeb.uconn.edu/lewishome
  Email: paul.lewis@uconn.edu


Versions after 2.0 contain changes primarily made by:
  Mark T. Holder  mholder@users.sourceforge.net

Other contributors to these versions include: Derrick Zwickl, Brian O'Meara, Brandon Chisham, François Michonneau, and Jeet Sukumaran

The code in examples/phylobase... was written by Brian O'Meara and Derrick Zwickl
for phylobase.

David Suárez Pascal contributed SWIG bindings which heavily influenced those
   found in branches/v2.2. Thanks to David for blazing the way on the swig binding,
    Google for funding, and NESCent (in particular Hilmar Lapp) for getting the
    NESCent GSoC program going.

The 2010 GSoC effort also led to enhancements in terms of annotation storage and
xml parsing which are currently on. Michael Elliot contributed some code to the branches/xml branch.
Thanks to NESCent and  Google for supporting that work.

Many of the files used for testing were provided by Arlin Stoltzfus (see
http://www.molevol.org/camel/projects/nexus/ for more information), the Mesquite
package, and from TreeBase (thanks, Bill Piel!).

