#ifndef FLATDB_SUBPROCEDURE_HPP_
#define FLATDB_SUBPROCEDURE_HPP_

#include <memory>
#include <map>
#include <vector>
#include "../apiDb/db.hpp"

#include "../debug.hpp"

namespace flatDb {

	template<typename tKey> class ProcedureT;


	// there is no synchronization in this classes

	template<typename tKey>
	class SubProcedureT
	{
	public:
		typedef std::shared_ptr<SubProcedureT<tKey>> SP;
		typedef RecordT<tKey> Record;
		typedef ProcedureT<tKey> Procedure;
		typedef std::function<uint8_t*(unsigned size)> tAllocateBufferFunction;
		unsigned const priority;
	protected:
		Procedure * fOwner;
	public:
		SubProcedureT(Procedure * owner, unsigned pPrioriy) : priority(pPrioriy), fOwner(owner) {}
		virtual ~SubProcedureT();
		// ----- method to process data from the database, return true if some changes were made
		virtual void process(std::vector<std::pair<tKey,uint8_t const *>> const & data) { throw std::logic_error("Not implemented"); } // TODO
		virtual bool process(std::vector<std::pair<tKey,uint8_t const *>> & data, tAllocateBufferFunction callback) { throw std::logic_error("Not implemented"); } // TODO
		// -----
		virtual bool isReadOnly() const = 0;
	};


	template<typename tKey>
	class SubProcedureReadRecordsFromRangeT : public SubProcedureT<tKey>
	{
	public:
		typedef RecordT<tKey> Record;
		typedef ProcedureT<tKey> Procedure;
		typedef SubProcedureT<tKey> SubProcedure;
	public:
		SubProcedureReadRecordsFromRangeT(Procedure * owner)
		: SubProcedure(owner, owner->priority) {}
		void process(std::vector<std::pair<tKey,uint8_t const *>> const & data) override final;
		bool isReadOnly() const override final { return true; }
	};


	template<typename tKey>
	class SubProcedureReadRecordsByKeysT : public SubProcedureT<tKey>
	{
	public:
		typedef RecordT<tKey> Record;
		typedef ProcedureT<tKey> Procedure;
		typedef SubProcedureT<tKey> SubProcedure;
	private:
		std::vector<Record*> fRecords;
		typename Record::tReadByKeyFunction fCallback;
	public:
		SubProcedureReadRecordsByKeysT
		(Procedure * owner, typename Record::tReadByKeyFunction callback, std::vector<Record*> & records)
		: SubProcedure(owner, owner->priority), fRecords(records), fCallback(callback) {}
		void process(std::vector<std::pair<tKey,uint8_t const *>> const & data) override final;
		bool isReadOnly() const override final { return true; }
	};


	template<typename tKey>
	class SubProcedureUpdateRecordsByKeysT : public SubProcedureT<tKey>
	{
	public:
		typedef std::shared_ptr<SubProcedureT<tKey>> SP;
		typedef RecordT<tKey> Record;
		typedef ProcedureT<tKey> Procedure;
		typedef SubProcedureT<tKey> SubProcedure;
		typedef std::function<uint8_t*(unsigned size)> tAllocateBufferFunction;
	private:
		std::vector<Record*> fRecords;
		typename Record::tUpdateByKeyFunction fCallback;
	public:
		SubProcedureUpdateRecordsByKeysT
		(Procedure * owner, typename Record::tUpdateByKeyFunction callback, std::vector<Record*> & records)
		: SubProcedure(owner, owner->priority), fRecords(records), fCallback(callback) {}
		bool process(std::vector<std::pair<tKey,uint8_t const *>> & data, tAllocateBufferFunction callback) override final;
		bool isReadOnly() const override final { return false; }
	};

}

#endif /* FLATDB_SUBPROCEDURE_HPP_ */
