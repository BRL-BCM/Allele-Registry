
CXX=g++
CXXFLAGS+=-Wall -std=c++11 -O3 #-I../kyotocabinet/include -I../rocksdb/include -I../genomeDb

LIB_KC=-L../kyotocabinet/lib/ -lkyotocabinet -lz
LIB_ROCKSDB=-L../rocksdb/lib/ -lrocksdb -lz -lbz2 -lsnappy
LIB_GENOMEDB=-L../genomeDb/ -lgenomeDb
LIB_LMDB=-L./liblmdb -Wl,-Bstatic -llmdb -Wl,-Bdynamic

BINARIES=test_write2_rand_lmdb  test_write2_seq_lmdb  test_read2_rand_lmdb  test_read2_seq_lmdb
#BINARIES=test_write_seq_dbKcHash test_write_seq_dbKcHash2 test_write_seq_dbKcHash4x
#BINARIES+=test_write_rand_dbKcHash test_write_rand_dbKcHash2 test_write_rand_dbKcHash4x
#BINARIES+=test_write_seq_dbKcTree test_write_rand_dbKcTree test_write_seq_dbKcTree2 test_write_rand_dbKcTree2
#BINARIES+=test_write_rand_dbRocks test_write_seq_dbRocks
#BINARIES+=test_write_rand_genomeDb test_write_seq_genomeDb
#BINARIES+=test_write2_rand_genomeDb test_write2_seq_genomeDb
#BINARIES+=test_write2_rand_dbKcHash test_write2_seq_dbKcHash
#BINARIES+=test_write2_rand_dbKcTree test_write2_seq_dbKcTree

.PHONY: all clean

all: $(BINARIES)

clean:
	-rm *.o  $(BINARIES)
	

test_write2_rand_lmdb: test_write2_rand.o dbLmdb.o
	$(CXX) -o $@ $^ $(LIB_LMDB) -pthread

test_write2_seq_lmdb: test_write2_seq.o dbLmdb.o
	$(CXX) -o $@ $^ $(LIB_LMDB) -pthread

test_read2_rand_lmdb: test_read2_rand.o dbLmdb.o
	$(CXX) -o $@ $^ $(LIB_LMDB) -pthread

test_read2_seq_lmdb: test_read2_seq.o dbLmdb.o
	$(CXX) -o $@ $^ $(LIB_LMDB) -pthread




test_write_seq_dbKcHash: test_write_seq.o dbKcHash.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_seq_dbKcHash2: test_write_seq.o dbKcHash2.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_seq_dbKcHash4x: test_write_seq.o dbKcHash4x.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_rand_dbKcHash: test_write_rand.o dbKcHash.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_rand_dbKcHash2: test_write_rand.o dbKcHash2.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread
	
test_write_rand_dbKcHash4x: test_write_rand.o dbKcHash4x.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_seq_dbKcTree: test_write_seq.o dbKcTree.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_rand_dbKcTree: test_write_rand.o dbKcTree.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_seq_dbKcTree2: test_write_seq.o dbKcTree2.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write_rand_dbKcTree2: test_write_rand.o dbKcTree2.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread
	
test_write_seq_dbRocks: test_write_seq.o dbRocks.o
	$(CXX) -o $@ $^ $(LIB_ROCKSDB) -pthread

test_write_rand_dbRocks: test_write_rand.o dbRocks.o
	$(CXX) -o $@ $^ $(LIB_ROCKSDB) -pthread

test_write_seq_genomeDb: test_write_seq.o dbGenome.o
	$(CXX) -o $@ $^ $(LIB_GENOMEDB)

test_write_rand_genomeDb: test_write_rand.o dbGenome.o
	$(CXX) -o $@ $^ $(LIB_GENOMEDB)

test_write2_seq_genomeDb: test_write2_seq.o dbGenome.o
	$(CXX) -o $@ $^ $(LIB_GENOMEDB)

test_write2_rand_genomeDb: test_write2_rand.o dbGenome.o
	$(CXX) -o $@ $^ $(LIB_GENOMEDB)

test_write2_seq_dbKcHash: test_write2_seq.o dbKcHash.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write2_rand_dbKcHash: test_write2_rand.o dbKcHash.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write2_seq_dbKcTree: test_write2_seq.o dbKcTree.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread

test_write2_rand_dbKcTree: test_write2_rand.o dbKcTree.o
	$(CXX) -o $@ $^ $(LIB_KC) -pthread
	
#test_write_seq_dbKcDir: test_write_seq.o dbKcDir.o
#	$(CXX) -o $@ $^ $(LIB_KC) -pthread
#
#test_write_rand_dbKcDir: test_write_rand.o dbKcDir.o 
#	$(CXX) -o $@ $^ $(LIB_KC) -pthread
 
