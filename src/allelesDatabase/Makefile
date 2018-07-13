include ../Makefile.globals

BINARIES=libAllelesDatabase.a  
#BINARIES+=test_TableGenomic_lmdb   test_TableProtein_lmdb   test_allelesDatabase_lmdb  
BINARIES+=test_TableGenomic_flat   test_TableProtein_flat   test_allelesDatabase_flat
#BINARIES+=test_TableGenomic_prefix test_TableProtein_prefix test_allelesDatabase_prefix

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)
	
libAllelesDatabase.a: allelesDatabase.o  IndexGenomicComplex.o IndexIdentifierCa.o IndexIdentifierPa.o IndexIdentifierUInt32.o RecordVariant.o TableGenomic.o TableProtein.o TableSequence.o tools.o
	ar -r $@ $^


test_TableGenomic_lmdb: test_TableGenomic.o TableGenomic.o RecordVariant.o tools.o $(DEP_LMDB_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_LMDB_DB) -Wl,-Bdynamic -pthread

test_TableProtein_lmdb: test_TableProtein.o TableProtein.o RecordVariant.o tools.o $(DEP_LMDB_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_LMDB_DB) -Wl,-Bdynamic -pthread

test_allelesDatabase_lmdb: test_allelesDatabase.o  libAllelesDatabase.a $(DEP_LMDB_DB)
	$(MAKE_BIN) $(LIB_LMDB_DB) $(LIB_REFERENCES_DATABASE) -Wl,-Bdynamic -pthread -lrt


test_TableGenomic_flat: test_TableGenomic.o TableGenomic.o RecordVariant.o tools.o $(DEP_FLAT_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_FLAT_DB) -Wl,-Bdynamic -pthread

test_TableProtein_flat: test_TableProtein.o TableProtein.o RecordVariant.o tools.o $(DEP_FLAT_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_FLAT_DB) -Wl,-Bdynamic -pthread

test_allelesDatabase_flat: test_allelesDatabase.o  libAllelesDatabase.a $(DEP_FLAT_DB)
	$(MAKE_BIN) $(LIB_FLAT_DB) $(LIB_REFERENCES_DATABASE) -Wl,-Bdynamic -pthread -lrt


test_TableGenomic_prefix: test_TableGenomic.o TableGenomic.o RecordVariant.o tools.o
	$(MAKE_BIN) $(LIB_CORE) $(LIB_PREFIX_TREE_DB) -Wl,-Bdynamic -pthread

test_TableProtein_prefix: test_TableProtein.o TableProtein.o RecordVariant.o tools.o
	$(MAKE_BIN) $(LIB_CORE) $(LIB_PREFIX_TREE_DB) -Wl,-Bdynamic -pthread

test_allelesDatabase_prefix: test_allelesDatabase.o  libAllelesDatabase.a
	$(MAKE_BIN) $(LIB_PREFIX_TREE_DB) $(LIB_REFERENCES_DATABASE) -Wl,-Bdynamic -pthread -lrt
