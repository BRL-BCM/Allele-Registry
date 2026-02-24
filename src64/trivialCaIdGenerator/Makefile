include ../Makefile.globals

BINARIES=caIdGeneratorDaemon test
INSTALL_BINARIES=caIdGeneratorDaemon

.PHONY: all clean

all: $(BINARIES)

clean:
	rm -f *.o $(BINARIES)

caIdGeneratorDaemon: caIdGeneratorUtility.o caIdGeneratorDaemon.o
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a ../allelesDatabase/libAllelesDatabase.a ../core/libCore.a ../flatDb/libFlatDb.a -lssl -lcrypto

test: test.o
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a ../allelesDatabase/libAllelesDatabase.a ../core/libCore.a ../flatDb/libFlatDb.a -lssl -lcrypto

DESTDIR ?= .
install:
	install -m 750 $(INSTALL_BINARIES) $(DESTDIR)
