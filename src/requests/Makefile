include ../Makefile.globals


BINARIES=libRequests.a test_hgvs test_canonicalization test_proteins test_OutputFormatter test_smithWaterman test_proteinVariantsToGenomic testGenerator

.PHONY: all clean

all: $(BINARIES)


clean:
	-rm *.o  $(BINARIES)


REQUESTS=Request
REQUESTS+=RequestFetchAlleleById
REQUESTS+=RequestFetchAlleleByHgvs
REQUESTS+=RequestFetchAllelesByDefinition
REQUESTS+=RequestFetchGene
REQUESTS+=RequestFetchReference
REQUESTS+=RequestQueryAlleles
REQUESTS+=RequestQueryAllelesById
REQUESTS+=RequestQueryAllelesByGene
REQUESTS+=RequestQueryAllelesByGeneAndMutation
REQUESTS+=RequestQueryAllelesByRegion
REQUESTS+=RequestQueryGenes
REQUESTS+=RequestQueryReferences
REQUESTS+=RequestDeleteAlleles
REQUESTS+=RequestAnnotateVcf
REQUESTS+=RequestQueryExternalSources
REQUESTS+=RequestCreateExternalSource
REQUESTS+=RequestModifyExternalSource
REQUESTS+=RequestDeleteExternalSources
REQUESTS+=RequestModifyExternalSourceUser
REQUESTS+=RequestModifyLink
REQUESTS+=RequestQueryAllelesByExternalSource
REQUESTS+=RequestDeleteExternalSourceLinks
REQUESTS+=RequestDeleteIdentifiers

	
libRequests.a: $(REQUESTS:=.o) OutputFormatter.o parsers.o identifiersTools.o proteins.o proteinVariantsToGenomic.o smithWaterman.o canonicalization.o hgvs.o
	ar -r $@ $^

test_hgvs: hgvs.o test_hgvs.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)
	
test_canonicalization: hgvs.o canonicalization.o smithWaterman.o test_canonicalization.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)

test_proteins: hgvs.o canonicalization.o proteins.o smithWaterman.o test_proteins.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)

test_OutputFormatter: test_OutputFormatter.o OutputFormatter.o identifiersTools.o canonicalization.o smithWaterman.o hgvs.o $(DEP_REFERENCES_DATABASE) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE) $(LIB_COMMON_TOOLS)

test_smithWaterman: test_smithWaterman.o smithWaterman.o $(DEP_CORE)
	$(MAKE_BIN) $(LIB_CORE)

test_proteinVariantsToGenomic: test_proteinVariantsToGenomic.o proteinVariantsToGenomic.o
	$(MAKE_BIN) $(LIB_CORE)

testGenerator: testGenerator.o
	$(MAKE_BIN) $(LIB_CORE)
