include ../Makefile.globals

CXXFLAGS+=-I../dependencies/yaml/include

BINARIES=libFlatDb.a  readIndexNode


.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)

libFlatDb.a: FlatDb.o Convert.o Scheduler.o IndexNode.o DataNode.o TasksManager.o Procedure.o SubProcedure.o FileWithPages.o StorageWithCache.o
	ar -r $@ $^

readIndexNode: readIndexNode.o
	$(CXX) $(CXXFLAGS) -o $@ $^  -lboost_system
