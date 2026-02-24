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
REQUESTS+=RequestCoordinateTransformation
REQUESTS+=RequestCoordinateTransformations
REQUESTS+=RequestSequenceServiceInfo
REQUESTS+=RequestSequenceByDigest
REQUESTS+=RequestVrAlleleForHgvs
REQUESTS+=RequestMetadataForSequenceByDigest
REQUESTS+=RequestGeneIntersection
REQUESTS+=RequestQueryAlleleIntersection
REQUESTS+=RequestBumpId
REQUESTS+=RequestBumpIds
REQUESTS+=RequestSetIds

	
libRequests.a: $(REQUESTS:=.o) OutputFormatter.o parsers.o identifiersTools.o proteins.o proteinVariantsToGenomic.o smithWaterman.o smithWaterman_full.o canonicalization.o hgvs.o coordinateTransformationHelper.o
	ar -r $@ $^

coordinateTransformationHelper: hgvs.o test_hgvs.o canonicalization.o OutputFormatter.o smithWaterman.o smithWaterman_full.o

test_hgvs: hgvs.o test_hgvs.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)
	
test_canonicalization: hgvs.o canonicalization.o smithWaterman.o smithWaterman_full.o test_canonicalization.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)

test_proteins: hgvs.o canonicalization.o proteins.o smithWaterman.o smithWaterman_full.o test_proteins.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)

test_OutputFormatter: test_OutputFormatter.o OutputFormatter.o identifiersTools.o canonicalization.o smithWaterman.o smithWaterman_full.o hgvs.o $(DEP_REFERENCES_DATABASE) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE) $(LIB_COMMON_TOOLS)

test_smithWaterman: test_smithWaterman.o smithWaterman.o smithWaterman_full.o $(DEP_CORE)
	$(MAKE_BIN) $(LIB_CORE)

test_proteinVariantsToGenomic: test_proteinVariantsToGenomic.o proteinVariantsToGenomic.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_CORE) $(LIB_REFERENCES_DATABASE)

testGenerator: testGenerator.o
	$(MAKE_BIN) $(LIB_CORE)
