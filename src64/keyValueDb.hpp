#ifndef KEYVALUEDB_HPP_
#define KEYVALUEDB_HPP_

#include <string>
#include <vector>

namespace keyValueDb {

	template<class Record>
	class Database {
	private:
		struct Pim;
		Pim * pim;
	public:
		Database(std::string const & path);
		~Database();
		void startTransaction();
		void commitTransaction();
		void getRecordsValues(std::vector<Record> &);
		void saveRecords(std::vector<Record> const &);
	};

	class Buffer {
	public:
		void * pointer;
		unsigned size;
		unsigned position;
	public:
		Buffer(unsigned initialSize);
		~Buffer();
		void writeBytes(unsigned long long value, unsigned bytesNumber);
		void writeString(std::string const & value);
//		void writeStringWithZero(std::string const & value);
		void readBytes(unsigned long long & value, unsigned bytesNumber);
		void readBytes(unsigned & value, unsigned bytesNumber);
//		void readStringWithZero(std::string & value);
		void readString(std::string & value, unsigned bytesNumber);
	};



}


#endif /* KEYVALUEDB_HPP_ */
