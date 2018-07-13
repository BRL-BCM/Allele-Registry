include ../Makefile.globals


BINARIES=generator
#BINARIES+=      lmdbDb_createAndCompare       lmdbDb_compare       lmdbDb_readAll       lmdbDb_addRecords       lmdbDb_sessions
BINARIES+=flatDb_createAndCompare flatDb_compare flatDb_readAll flatDb_addRecords flatDb_sessions flatDb_test1 flatDb_test2
#BINARIES+=prefixTreeDb_createAndCompare prefixTreeDb_compare prefixTreeDb_readAll prefixTreeDb_addRecords prefixTreeDb_sessions


.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)


generator: generator.o
	$(CXX) -Wall -o $@ $^  


lmdbDb_createAndCompare: testDb_createAndCompare.o $(DEP_LMDB_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_LMDB_DB)
flatDb_createAndCompare: testDb_createAndCompare.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_FLAT_DB)
prefixTreeDb_createAndCompare: testDb_createAndCompare.o $(DEP_PREFIX_TREE_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_PREFIX_TREE_DB)


lmdbDb_compare: testDb_compare.o $(DEP_LMDB_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_LMDB_DB)
flatDb_compare: testDb_compare.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_FLAT_DB)
prefixTreeDb_compare: testDb_compare.o $(DEP_PREFIX_TREE_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_PREFIX_TREE_DB)


lmdbDb_readAll: testDb_readAll.o $(DEP_LMDB_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_LMDB_DB)
flatDb_readAll: testDb_readAll.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_FLAT_DB)
prefixTreeDb_readAll: testDb_readAll.o $(DEP_PREFIX_TREE_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_PREFIX_TREE_DB)
	
	
lmdbDb_addRecords: testDb_addRecords.o $(DEP_LMDB_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_LMDB_DB)
flatDb_addRecords: testDb_addRecords.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_FLAT_DB)
prefixTreeDb_addRecords: testDb_addRecords.o $(DEP_PREFIX_TREE_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_PREFIX_TREE_DB)


lmdbDb_sessions: testDb_sessions.o $(DEP_LMDB_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_LMDB_DB)
flatDb_sessions: testDb_sessions.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread  $(LIB_FLAT_DB)
prefixTreeDb_sessions: testDb_sessions.o $(DEP_PREFIX_TREE_DB)
	$(CXX) -Wall -o $@ $^ -pthread $(LIB_PREFIX_TREE_DB)
	
flatDb_test1: testDb_test1.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread  $(LIB_FLAT_DB)
flatDb_test2: testDb_test2.o $(DEP_FLAT_DB)
	$(CXX) -Wall -o $@ $^ -pthread  $(LIB_FLAT_DB)
	
