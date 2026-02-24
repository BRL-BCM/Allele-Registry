include ../Makefile.globals


BINARIES=test_TasksManager  test_PagesManager  
BINARIES+=libPrefixTreeDb.a  readIndexNode  getIndexNodesStat

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)

test_PagesManager: test_PagesManager.o PagesManager.o FileWithPages.o
	$(CXX) -Wall -o $@ $^

test_TasksManager: test_TasksManager.o TasksManager.o
	$(CXX) -Wall -o $@ $^ -pthread

libPrefixTreeDb.a: PrefixTreeDb.o IndexNode.o TasksManager.o PagesManager.o FileWithPages.o
	ar -r $@ $^

readIndexNode: readIndexNode.o
	$(CXX) -Wall -o $@ $^  -lboost_system

getIndexNodesStat: getIndexNodesStat.o
	$(CXX) -Wall -o $@ $^  -lboost_system
