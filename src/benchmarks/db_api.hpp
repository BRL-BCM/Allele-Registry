#ifndef DB_API_HPP_
#define DB_API_HPP_

#include <cstdint>
#include <vector>

struct Record
{
	uint32_t key;
	uint64_t data1;
	uint16_t data2;
	bool operator<(Record const & r) const
	{
		if (key != r.key) return (key < r.key);
		if (data1 != r.data1) return (data1 < r.data1);
		return (data2 < r.data2);
	}
};

class DB
{
private:
	struct Pim;
	Pim * pim;
public:
	DB();
	~DB();
	void set(std::vector<Record> & keysValues);
	void getAndValidate(std::vector<Record> & keysValues);
};



#endif /* DB_API_HPP_ */
