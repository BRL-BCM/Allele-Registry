include Makefile.globals

# for YAML and httpd
CXXFLAGS+=-I./dependencies/yaml/include -I.

LIB_HTTP=-L. -lhttpp -lboost_thread -lssl -lcrypto 
LIB_BOOST=-lboost_filesystem -lboost_system -lboost_timer -lboost_chrono -lboost_log
LIB_YAML=-L./dependencies/yaml/lib -lyaml-cpp
#LIB_C=-Wl,-Bdynamic -pthread -lrt -ldl -Wl,-Bstatic 

DIRECTORIES=commonTools mysql flatDb apiDb core referencesDatabase allelesDatabase externalSources requests  # prefixTreeDb lmdbDb
BINARIES=alleleRegistry shm_cleaner test_server exac_extract_ids exac_compare_ids myVariantInfo_compare_ids myVariantInfo_sort buildAlignFile


.PHONY: all clean $(DIRECTORIES)


all: $(DIRECTORIES) $(BINARIES)


clean: $(DIRECTORIES)
	-rm *.o  $(BINARIES)

$(DIRECTORIES):
	$(MAKE) -C $@  $(MAKECMDGOALS)

	
alleleRegistry: libhttpp.a server.o dispatcher.o authorization.o $(DEP_REQUESTS) $(DE $(DEP_MYSQL) $(DEP_COMMON_TOOLS)
	$(MAKE_BIN) $(LIB_REQUESTS) $(LIB_MYSQL) $(LIB_COMMON_TOOLS) $(LIB_HTTP) $(LIB_YAML) -lboost_log
	
test_server: libhttpp.a server.o dispatcher_echo.o $(DEP_COMMON_TOOLS) 
	$(MAKE_BIN) $(LIB_COMMON_TOOLS) $(LIB_HTTP) $(LIB_BOOST) $(LIB_YAML) -Wl,-Bdynamic -pthread -ldl -lrt

shm_cleaner: shm_cleaner.o
	$(MAKE_BIN) -lboost_system -Wl,-Bdynamic -pthread -lrt

exac_extract_ids: exac_extract_ids.o $(DEP_CORE)
	$(MAKE_BIN) $(LIB_CORE)
	
exac_compare_ids: exac_compare_ids.o
	$(MAKE_BIN)
	
myVariantInfo_compare_ids: myVariantInfo_compare_ids.o
	$(MAKE_BIN)

myVariantInfo_sort: myVariantInfo_sort.o
	$(MAKE_BIN)

libhttpp.a:
	make -C ./httpp

buildAlignFile: buildAlignFile.o $(DEP_REFERENCES_DATABASE)
	$(MAKE_BIN) $(LIB_REFERENCES_DATABASE)

