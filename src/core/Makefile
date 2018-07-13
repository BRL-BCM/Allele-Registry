include ../Makefile.globals

BINARIES=libCore.a test_myvariantinfo

.PHONY: all clean

all: $(BINARIES)


clean:
	-rm *.o  $(BINARIES)
	
libCore.a: document.o exceptions.o globals.o identifiers.o variants.o textLabels.o externalSources.o
	ar -r $@ $^

test_myvariantinfo: test_myvariantinfo.o globals.o
	$(MAKE_BIN)