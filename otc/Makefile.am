pkglib_LTLIBRARIES = libotcetera.la
libotcetera_ladir = $(includedir)/otc
libotcetera_la_LDFLAGS = -release @PACKAGE_VERSION@


libotcetera_la_HEADERS = \
	embedding_cli.h \
	error.h \
	ftree.h \
	newick.h \
	otcetera.h \
	otc_base_includes.h \
	otcli.h \
	test_harness.h \
	tree.h \
	tree_data.h \
	tree_iter.h \
	tree_operations.h \
	tree_util.h \
	util.h

libotcetera_la_SOURCES = \
	embedded_tree.cpp \
	forest.cpp \
	ftree.cpp \
	greedy_forest.cpp \
	newick.cpp \
	node_embedding.cpp \
	otcetera.cpp \
	otcli.cpp \
	supertree_util.cpp \
	test_harness.cpp \
	tree.cpp \
	util.cpp \
	write_dot.cpp