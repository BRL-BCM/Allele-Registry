include ../Makefile.globals


BINARIES=libLmdbDb.a
#BINARIES+=test_TasksManager  test_PagesManager  
#BINARIES+=readIndexNode  getIndexNodesStat
#BINARIES+=generator  testDb_createAndCompare  testDb_compare  testDb_readAll  testDb_addRecords  

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)

libLmdbDb.a:  db.o  ../lmdb/mdb.o  ../lmdb/midl.o TasksManager.o
	ar -r $@ $^
