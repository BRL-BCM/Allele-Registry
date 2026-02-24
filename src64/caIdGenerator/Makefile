include ../Makefile.globals

BINARIES=caIdGeneratorDaemon consumer test
INSTALL_BINARIES=caIdGeneratorDaemon

.PHONY: all clean

all: $(BINARIES)

clean:
	rm -f *.o $(BINARIES)

caIdGeneratorDaemon: caIdGeneratorUtility.o caIdGeneratorDaemon.o
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a

consumer: consumer.o
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a

test: test.o
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a

DESTDIR ?= .
install:
	install -m 750 $(INSTALL_BINARIES) $(DESTDIR)
