include ../Makefile.globals

BINARIES=libExternalSources.a  
BINARIES+=test_externalSources

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)
	
libExternalSources.a: externalSources.o  docBuilder.o  db.o  core.o
	ar -r $@ $^

test_externalSources: test_externalSources.o  libExternalSources.a  $(DEP_CORE) $(DEP_COMMON_TOOLS)  $(DEP_MYSQL)
	$(MAKE_BIN) $(LIB_CORE)  $(LIB_COMMON_TOOLS) $(LIB_MYSQL)
