include ../Makefile.globals

.PHONY: all clean

BINARIES=test_bytesLevel  test_fastString  libCommonTools.a

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)

test_bytesLevel: test_bytesLevel.o
	$(CXX) -Wall -o $@ $^

test_fastString: test_fastString.o
	$(CXX) -Wall -o $@ $^

test_json: test_json.o json.o
	$(MAKE_BIN)

libCommonTools.a: Stopwatch.o json.o JsonBuilder.o DocumentSettings.o
	ar -r $@ $^

	
test_fastString.o: fastString.hpp
