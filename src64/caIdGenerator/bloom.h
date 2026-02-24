#ifndef BLOOM_H
#define BLOOM_H

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <sstream>
#include <fstream>
#include <cstring>
#include <iostream>

class HashBuffer {

private:
	const std::string FILENAME; // save / restore filename
	const unsigned SIZE; // size of buffer in bytes
	const unsigned NUM; // size of index in bytes

	uint8_t * buffer;

	void initialize() {
		buffer = new uint8_t[SIZE];
		memset(buffer, 0, SIZE);
	}

public:
	HashBuffer( std::string pfilename
	          , unsigned psize
	          , unsigned pnum = 4
	          )
	: FILENAME(pfilename)
	, SIZE(psize)
	, NUM(pnum)
	{
		initialize();
		restore();
	}

	~HashBuffer() {
		delete [] buffer;
	}

	////////////////////////////////////////////////////////////////////////////////////////
	//
	//   str - sequence of size NUM that represents the index into buffer
	//   note: there is a relationship between SIZE and NUM
	//   a buffer of size SIZE has the largest possible index of 8*SIZE 
	//   (8, since we can store bools into bits)
	//   an index of size NUM bytes, can represent a maximum value of 2^(8*NUM)-1
	//   for instance, the default value for NUM is 4 (4 bytes = UInt32)
	//   so that's 2^(8*4) = 2^(32) = 4,294,967,296
	//   so the maximum size of the buffer that can be indexed is
	//   (2^(32)-1) = 4,294,967,296 bits = 536,870,912 bytes = 536.9 Mb for one buffer
	//   for a key of size 32 bytes, we would need 32/4 = 8 buffers = 4 Gb of total space
	//
	////////////////////////////////////////////////////////////////////////////////////////
	inline
	bool get(const uint8_t * key) const {
		const uint32_t index = *reinterpret_cast<const uint32_t*>(key);
		uint8_t & b = buffer[index >> 3]; // index / 8
		// possible parallel read
		return (b & (1 << (index%8))) != 0;
	}

	inline
	void set(const uint8_t * key) {
		const uint32_t index = *reinterpret_cast<const uint32_t*>(key);
		uint8_t & b = buffer[index >> 3]; // index / 8
		// possible parallel write
		// should mutex this {
		b |= (1 << (index%8));
		// {
	}

	inline
	bool get(const uint32_t index) const {
		uint8_t & b = buffer[index >> 3]; // index / 8
		// possible parallel read
		return (b & (1 << (index%8))) != 0;
	}

	inline
	void set(const uint32_t index) {
		uint8_t & b = buffer[index >> 3]; // index / 8
		// possible parallel write
		// should mutex this {
		b |= (1 << (index%8));
		// {
	}

	void save() const {
		std::ofstream wf(FILENAME.c_str(), std::ios::out | std::ios::binary);
		if(!wf) {
			std::cout << "cannot open file: " << FILENAME << std::endl;
			return;
		}
		wf.write((char*)&buffer[0], SIZE);
		wf.close();
		if(!wf.good()) {
			std::cout << "error occurred at writing time: " << FILENAME << std::endl;
		}
	}

	void restore() {
		std::ifstream rf(FILENAME.c_str(), std::ios::in | std::ios::binary);
		if(!rf) {
			return;
		}
		rf.read((char*)&buffer[0], SIZE);
		rf.close();
		if(!rf.good()) {
			std::cout << "error occurred at reading time: " << FILENAME << std::endl;
		}
	}

};

class BloomDaemon {

private:
	const std::string FILENAME; 
	const unsigned INDEX_SIZE; // in bytes
	const unsigned NUM;
	const unsigned BUFFER_KEY_SIZE = 4;

	boost::mutex io_mutex;

	HashBuffer ** dat;

	void initialize(unsigned psize) {
		dat = new HashBuffer*[NUM];
		for(unsigned i=0; i<NUM; i++) {
			std::stringstream ss;
			ss << FILENAME << "-" << i << ".dat";
			dat[i] = new HashBuffer(ss.str(), psize);
		}
	}

public:
	// pnum - number of buffers
	// psize - size of each buffer
	BloomDaemon( std::string pfilename // save / restore filename
	           , unsigned pnum = 64 // INDEX_SIZE / BUFFER_KEY_SIZE
	           , unsigned pindexSize = 32 // note should be 
	                                      // an exact multiple of 4 (BUFFER_KEY_SIZE)
	           , unsigned psize = 536870912
	           )
	: FILENAME(pfilename)
	, INDEX_SIZE(pindexSize)
	, NUM(pnum)
	{
		initialize(psize);
	}

	inline
	bool get(const uint32_t & id, const uint32_t & key) const {
		return dat[id]->get(key);
	}

	inline
	bool get(const uint8_t * key) const {
		int offset = 0;
		for(unsigned i=0; i<NUM; i++, offset+=BUFFER_KEY_SIZE) {
			if(dat[i]->get(&key[offset]) == false) {
				return false;
			}
		}
		// assert offset == INDEX_SIZE
		return true;
	}

	inline
	bool get(const char * key) const {
		return get(reinterpret_cast<const uint8_t *>(key));
	}

	inline
	bool get(std::string const & str) const {
		return get(str.c_str());
	}
	void fget(int i, std::vector<std::pair<uint32_t, uint32_t> > * pvec) {
		std::sort(pvec->begin(), pvec->end(), [](typename std::pair<uint32_t, uint32_t> const & a, typename std::pair<uint32_t, uint32_t> const & b)->bool{return a.second < b.second;});
		for(auto x : (*pvec)) {
			x.second = dat[i]->get(x.second);
		}
	}

	inline
	void pget(std::vector<uint64_t> & vec) {
		boost::mutex::scoped_lock scoped_lock(io_mutex);
		std::vector<std::vector<std::pair<uint32_t, uint32_t> > > pvec(NUM);
		for(unsigned k=0; k<vec.size(); k++) {
			const uint32_t * i = reinterpret_cast<const uint32_t *>(&vec[k]);
			pvec[i[1]%NUM].push_back(std::pair<uint32_t, uint32_t>(k,i[0]));
		}
		std::vector<boost::thread *> threads;
		for(unsigned i=0; i<NUM; i++) {
			boost::thread * t = new boost::thread( std::bind( &BloomDaemon::fget, this, i, &pvec[i]) );
			threads.push_back(t);
		}
		for(unsigned i=0; i<threads.size(); i++) {
			threads[i]->join();
			for(auto x : pvec[i]) {
				vec[x.first] = x.second;
			}
			delete threads[i];
		}
		pvec.clear();
	}

	void fset(int i, std::vector<uint32_t> * pvec) {
		std::sort(pvec->begin(), pvec->end());
		for(auto x : (*pvec)) {
			dat[i]->set(x);
		}
	}

	inline
	void pset(std::vector<uint64_t> const & vec) {
		boost::mutex::scoped_lock scoped_lock(io_mutex);
		std::vector<std::vector<uint32_t> > pvec(NUM);
		for(unsigned k=0; k<vec.size(); k++) {
			const uint32_t * i = reinterpret_cast<const uint32_t *>(&vec[k]);
			pvec[i[1]%NUM].push_back(i[0]);
		}
		std::vector<boost::thread *> threads;
		for(unsigned i=0; i<NUM; i++) {
			boost::thread * t = new boost::thread( std::bind( &BloomDaemon::fset, this, i, &pvec[i]) );
			threads.push_back(t);
		}
		for(unsigned i=0; i<threads.size(); i++) {
			threads[i]->join();
			delete threads[i];
		}
		pvec.clear();
	}

	inline
	void set(std::vector<uint64_t> const & vec) {
		boost::mutex::scoped_lock scoped_lock(io_mutex);
		uint32_t minI = 4294967295L;
		uint32_t maxI = 0;
		for(uint64_t k = 0; k < vec.size(); k++) {
			const uint32_t * i = reinterpret_cast<const uint32_t *>(&vec[k]);
			set(i[1]%NUM,i[0]);
			minI = (minI<i[0])?minI:i[0];
			maxI = (maxI>i[0])?maxI:i[0];
		}
		std::cout << minI << "\t" << maxI << std::endl;
	}

	inline
	void set(const uint32_t & id, const uint32_t & key) const {
		dat[id]->set(key);
	}

	inline
	void set(const uint8_t * key) const {
		int offset = 0;
		for(unsigned i=0; i<NUM; i++, offset+=BUFFER_KEY_SIZE) {
			dat[i]->set(&key[offset]);
		}
		// assert offset == INDEX_SIZE
	}

	inline
	void set(const char * key) const {
		set(reinterpret_cast<const uint8_t *>(key));
	}

	inline
	void set(std::string const & str) const {
		set(str.c_str());
	}

	void save() const {
		for(unsigned i=0; i<NUM; i++) {
			dat[i]->save();
		}
	}

	void restore() const {
		for(unsigned i=0; i<NUM; i++) {
			dat[i]->restore();
		}
	}

	void fcount(int i, std::vector<uint64_t> * vcount) {
		uint64_t c = 0;
		for(uint64_t j=0; j<4294967296L; j++) {
			c += dat[i]->get((uint32_t)j);
			//if(i==0) if(j%1000000==0) std::cout << j << std::endl;
		}
		(*vcount)[i] = c;
	}

	uint64_t numSet() {
		boost::mutex::scoped_lock scoped_lock(io_mutex);
		std::vector<uint64_t> count(NUM);
		std::vector<boost::thread *> threads;
		for(unsigned i=0; i<NUM; i++) {
			boost::thread * t = new boost::thread( std::bind( &BloomDaemon::fcount, this, i, &count) );
			threads.push_back(t);
		}
		uint64_t c = 0;
		for(unsigned i=0; i<threads.size(); i++) {
			threads[i]->join();
			c += count[i];
			delete threads[i];
		}
		threads.clear();
		count.clear();
		return c;
	}

	uint64_t numTotal() {
		boost::mutex::scoped_lock scoped_lock(io_mutex);
		uint64_t count = 0;
		for(unsigned i=0; i<NUM; i++) {
			count += 4294967296L;
		}
		return count;
	}

};

#endif
