include ../Makefile.globals


BINARIES=libReferencesDatabase.a  test_alignment  test_referencesDatabase  test_referencesDatabase_full  test_compareSequences

.PHONY: all clean

all: $(BINARIES)


clean:
	-rm *.o  $(BINARIES)
	
referencesDatabase.o: tabSeparatedFile.hpp

libReferencesDatabase.a: alignment.o referencesDatabase.o 
	ar -r $@ $^

test_alignment: alignment.o  test_alignment.o
	$(MAKE_BIN) $(LIB_CORE)

test_referencesDatabase: alignment.o referencesDatabase.o test_referencesDatabase.o 
	$(MAKE_BIN) $(LIB_CORE) -lboost_filesystem -lboost_timer -lboost_chrono -lboost_system -Wl,-Bdynamic -pthread -lrt

test_referencesDatabase_full: alignment.o referencesDatabase.o test_referencesDatabase_full.o 
	$(MAKE_BIN) $(LIB_CORE) -lboost_filesystem -lboost_timer -lboost_chrono -lboost_system -Wl,-Bdynamic -pthread -lrt

test_compareSequences: alignment.o referencesDatabase.o test_compareSequences.o
	$(MAKE_BIN) $(LIB_CORE) -lboost_filesystem -lboost_timer -lboost_chrono -lboost_system -Wl,-Bdynamic -pthread -lrt