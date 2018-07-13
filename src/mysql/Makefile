include ../Makefile.globals


CXXFLAGS+=-I../dependencies/mysql/include


BINARIES=libMysql.a test_mysqlConnection


.PHONY: all clean


all: $(BINARIES)


clean:
	-rm *.o  $(BINARIES)


libMysql.a: mysqlConnection.o
	ar -r $@ $^
	
	
test_mysqlConnection: test_mysqlConnection.o mysqlConnection.o
	$(MAKE_BIN) -L../dependencies/mysql/lib -lmysqlclient -Wl,-Bdynamic -pthread -ldl -lrt -Wl,-Bstatic

	