include ../Makefile.globals


BINARIES=libFlatDb.a  readIndexNode


.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)

libFlatDb.a: FlatDb.o Scheduler.o IndexNode.o DataNode.o TasksManager.o Procedure.o SubProcedure.o FileWithPages.o StorageWithCache.o
	ar -r $@ $^

readIndexNode: readIndexNode.o
	$(CXX) -Wall -o $@ $^  -lboost_system
