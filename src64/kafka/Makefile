include ../Makefile.globals

BINARIES=libKafka.a sharedMemoryToDiskDaemon diskToKafkaDaemon test_kafkaConnection send receive
INSTALL_BINARIES=sharedMemoryToDiskDaemon diskToKafkaDaemon

.PHONY: all clean

all: $(BINARIES)

clean:
	rm -f *.o $(BINARIES)

libKafka.a: kafkaConnection.o
	ar -r $@ $^

test_kafkaConnection: test_kafkaConnection.o libKafka.a
	$(MAKE_BIN) -Wl,-Bdynamic -lrdkafka++ -lrdkafka -lstdc++ -lssl -lcrypto -pthread -lz -ldl

send: send.o
	$(MAKE_BIN) -lboost_serialization -lrt -pthread

receive: receive.o
	$(MAKE_BIN) -lboost_serialization -lrt -pthread

sharedMemoryToDiskDaemon: sharedMemoryToDiskUtility.o sharedMemoryToDiskDaemon.o libKafka.a
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -lrdkafka++ -lrdkafka -lstdc++ -lssl -lcrypto -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a

diskToKafkaDaemon: diskToKafkaUtility.o diskToKafkaDaemon.o libKafka.a
	$(MAKE_BIN) -Wl,-Bdynamic -lboost_serialization -lboost_filesystem -lrt -lrdkafka++ -lrdkafka -lstdc++ -lssl -lcrypto -pthread -lz -ldl -lyaml-cpp ../commonTools/libCommonTools.a


DESTDIR ?= .
install:
	install -m 750 $(INSTALL_BINARIES) $(DESTDIR)
