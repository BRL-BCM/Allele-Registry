#include <boost/serialization/string.hpp>
#include <boost/serialization/list.hpp>

#define MAX_SIZE 1000000
#define MAX_MSG 1000

class dataEntry {
	public:
		dataEntry (bool _type, uint64_t _id)
			: type(_type), id(_id)
			{}
		dataEntry ()
			: type(true), id(-1)
			{}
		bool getType() const {return type;}
		uint64_t getId() const {return id;}
	private:
		bool type;
		uint64_t id;
		friend class boost::serialization::access;
		template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				ar & type;
				ar & id;
			}
};

class sharedDataEntry
{
	public:
		// type = true: genomic
		// type = false: protein
		// id = <CA/PA> id
		sharedDataEntry () {};
		void add(bool type, uint64_t id) {data.push_back(dataEntry(type,id));};
		void clear() {data.clear();}
		void pop_front() {data.pop_front();}
		dataEntry const & front() {return data.front();}
		bool empty() {return data.empty();}
		std::list<dataEntry> const & getData() const {return data;}
		unsigned size() const {
			return data.size();
		}
		void add(sharedDataEntry const & a) {
			for(auto const & x:a.data) {
				data.push_back(x);
			}
		}
	private:
		std::list<dataEntry> data;
		friend class boost::serialization::access;
		template<class Archive>
			void serialize(Archive & ar, const unsigned int version)
			{
				ar & BOOST_SERIALIZATION_NVP(data);
			}
};
