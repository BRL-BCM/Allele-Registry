include ../Makefile.globals

BINARIES=libAllelesDatabase.a getDuplicateCaIdRecords
#BINARIES+=test_TableGenomic_lmdb   test_TableProtein_lmdb   test_allelesDatabase_lmdb  
#BINARIES+=test_TableGenomic_flat   test_TableProtein_flat   test_allelesDatabase_flat
#BINARIES+=test_TableGenomic_prefix test_TableProtein_prefix test_allelesDatabase_prefix
#BINARIES+=testNamed   convertCaId   convertPaId   convertProtein   convertSequence   convertIndexIdentifiers   convertGenomicDataNodeDeployment   convertGenomicIndexNodeDeployment   convertProteinDataNodeDeployment   convertProteinIndexNodeDeployment   convertCaIdIndexNodeDeployment   convertCaIdDataNodeDeployment

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm -f *.o  $(BINARIES)
	
libAllelesDatabase.a: allelesDatabase.o  IndexGenomicComplex.o IndexIdentifierCa.o IndexIdentifierPa.o IndexIdentifierUInt64.o RecordVariant64.o TableGenomic64.o TableProtein64.o TableNamed.o TableSequence.o TableProteinSequence.o tools.o
	ar -r $@ $^

test_TableGenomic_lmdb: test_TableGenomic.o TableGenomic.o RecordVariant.o tools.o $(DEP_LMDB_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_LMDB_DB) -Wl,-Bdynamic -pthread

test_TableProtein_lmdb: test_TableProtein.o TableProtein.o RecordVariant.o tools.o $(DEP_LMDB_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_LMDB_DB) -Wl,-Bdynamic -pthread

test_allelesDatabase_lmdb: test_allelesDatabase.o  libAllelesDatabase.a $(DEP_LMDB_DB)
	$(MAKE_BIN) $(LIB_LMDB_DB) $(LIB_REFERENCES_DATABASE) -Wl,-Bdynamic -pthread -lrt



test_TableGenomic_flat: test_TableGenomic.o IndexIdentifierCa.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) -Wl,-Bdynamic -pthread

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

convertCaId: convertCaId.o IndexIdentifierCa.o IndexIdentifierCa32.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertCaIdDataNodeDeployment: convertCaIdDataNodeDeployment.o IndexIdentifierCa.o IndexIdentifierCa32.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertCaIdIndexNodeDeployment: convertCaIdIndexNodeDeployment.o IndexIdentifierCa.o IndexIdentifierCa32.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertPaId: convertPaId.o TableProtein64.o IndexIdentifierCa.o IndexIdentifierCa32.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertProtein: convertProtein.o TableProtein64.o IndexIdentifierCa.o IndexIdentifierCa32.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

fetchCAIds: TableProtein64.o TableNamed.o fetchCAIds.o IndexIdentifierUInt64.o IndexIdentifierUInt32.o IndexIdentifierCa.o IndexIdentifierCa32.o IndexIdentifierPa.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

duplicateCAIds: TableProtein64.o TableNamed.o duplicateCAIds.o IndexIdentifierUInt64.o IndexIdentifierUInt32.o IndexIdentifierCa.o IndexIdentifierCa32.o IndexIdentifierPa.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) $(LIB_REFERENCES_DATABASE) $(LIB_REQUESTS) -Wl,-Bdynamic -pthread -lyaml-cpp

getDuplicateCaIdRecords: TableProtein64.o TableNamed.o getDuplicateCaIdRecords.o IndexIdentifierUInt64.o IndexIdentifierUInt32.o IndexIdentifierCa.o IndexIdentifierCa32.o IndexIdentifierPa.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp -lcrypto

convertIndexIdentifiers: TableProtein64.o convertIndexIdentifiers.o IndexIdentifierUInt64.o IndexIdentifierUInt32.o IndexIdentifierCa.o IndexIdentifierCa32.o IndexIdentifierPa.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertSequence: convertSequence.o TableSequence.o IndexIdentifierCa.o IndexIdentifierCa32.o TableGenomic64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertGenomicDataNodeDeployment: convertGenomicDataNodeDeployment.o TableGenomic32.o TableGenomic64.o RecordVariant32.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertGenomicIndexNodeDeployment: convertGenomicIndexNodeDeployment.o TableGenomic32.o TableGenomic64.o RecordVariant32.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertProteinDataNodeDeployment: convertProteinDataNodeDeployment.o TableProtein32.o TableProtein64.o RecordVariant32.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertProteinIndexNodeDeployment: convertProteinIndexNodeDeployment.o TableProtein32.o TableProtein64.o RecordVariant32.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

testNamed: testNamed.o TableNamed.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(CXXFLAGS) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertToTsv: convertToTsv.o TableNamed.o TableGenomic64.o TableProtein64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(CXXFLAGS) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertNamedToTsv: convertNamedToTsv.o TableNamed.o TableGenomic64.o TableProtein64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(CXXFLAGS) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp

convertClinVarToTsv: convertClinVarToTsv.o TableNamed.o TableGenomic64.o TableProtein64.o RecordVariant64.o tools.o $(DEP_FLAT_DB) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(CXXFLAGS) $(LIB_CORE) $(LIB_YAML) $(LIB_FLAT_DB) $(LIB_COMMON_TOOLS) -Wl,-Bdynamic -pthread -lyaml-cpp
