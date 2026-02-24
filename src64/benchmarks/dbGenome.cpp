#include "db_api.hpp"
#include "../genomeDb/FileWithPages.hpp"
#include <map>

#include <iostream>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define CHECK(x) { rocksdb::Status s = (x); if ( ! s.ok() ) throw std::runtime_error("Command failed: " TOSTRING(s.ToString())); }

unsigned const pageSize = 512;
unsigned const valuesPerPage = pageSize / 4;

struct DB::Pim {
	genomeDb::FileWithPages<pageSize> db;
	uint32_t * page;
	Pim(std::string const & path) : db(path) { page = new uint32_t[valuesPerPage]; }
};

DB::DB() : pim(new Pim("dbGenome.gdb"))
{}

DB::~DB()
{
	delete pim;
}

void DB::set(uint32_t key, std::vector<uint32_t> const & value)
{
	unsigned const pageIndex = key/valuesPerPage;
	pim->db.getPage(pageIndex, pim->page);
	pim->page[key%valuesPerPage] = value[0];
	pim->db.savePage(pageIndex, pim->page);
}

void DB::set(std::vector<std::pair<uint32_t,uint32_t>> const & keysValues)
{
	std::map<uint32_t, uint32_t*> pages;
	for (auto const & kv: keysValues) pages[kv.first/valuesPerPage] = nullptr;
	for (auto & kv: pages) kv.second = new uint32_t[valuesPerPage];
	for (auto & kv: pages) pim->db.getPage(kv.first, kv.second);
	for (auto const & kv: keysValues) pages[kv.first/valuesPerPage][kv.first%valuesPerPage] = kv.second;
	for (auto & kv: pages) pim->db.savePage(kv.first, kv.second);
	for (auto & kv: pages) delete [] kv.second;
}
