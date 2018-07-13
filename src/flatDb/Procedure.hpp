#ifndef FLATDB_PROCEDURE_HPP_
#define FLATDB_PROCEDURE_HPP_

#include "SubProcedure.hpp"
#include "Bin.hpp"
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace flatDb {

	template <typename tKey>
	class SchedulerT;


	template <typename tKey>
	class ProcedureT
	{
	public:
		typedef BinT<tKey> Bin;
		typedef SubProcedureT<tKey> SubProcedure;
		unsigned const priority;
		SchedulerT<tKey> * const scheduler;
	protected:
		// ----- tools to set procedure as completed
		std::mutex fAccessToCompleted;
		std::condition_variable fWaitingForCompleted;
		bool fCompleted = false;
		std::atomic<unsigned> fCounter;
		void announceAsCompleted(); // called from derived class
	private:
		// ----- api for subprocedure's destructor
		friend SubProcedure;
		void subProcedureWasDeleted();
		// ----- method called from subProcedureWasDeleted() when all subprocedures are completed
		virtual void allSubProceduresWereDeleted() = 0;
	public:
		// ----- constructors & destructor
		ProcedureT(SchedulerT<tKey> * pScheduler, unsigned pPriority) : priority(pPriority), scheduler(pScheduler), fCounter(0) {}
		virtual ~ProcedureT() {}
		// ----- blocks thread until procedure is completed
		void waitUntilCompleted();
		// ----- divide procedure into subprocedures, called from scheduler
		virtual std::map<unsigned,typename SubProcedure::SP> createSubProcedures(std::vector<Bin> const & entries) = 0;
	};


	template <typename tKey>
	class ProcedureReadRecordsFromRangeT : public ProcedureT<tKey>
	{
	public:
		typedef BinT<tKey> Bin;
		typedef RecordT<tKey> Record;
		typedef KeysRangeT<tKey> KeysRange;
		typedef SubProcedureT<tKey> SubProcedure;
		typedef SubProcedureReadRecordsFromRangeT<tKey> SubProcedureReadRecordsFromRange;
	private:
		tKey fFirstKey;
		tKey const fLastKey;
		bool fEndOfQuery = false;
		typename Record::tReadFromRangeFunction fCallback;
		void allSubProceduresWereDeleted() override final;
		// access for subprocedures
		friend SubProcedureReadRecordsFromRange;
		void process(std::vector<std::pair<tKey,uint8_t const *>> const & data);
	public:
		ProcedureReadRecordsFromRangeT
		(SchedulerT<tKey>* scheduler, unsigned priority, typename Record::tReadFromRangeFunction callback, tKey const & first, tKey const & last)
		: ProcedureT<tKey>(scheduler, priority), fFirstKey(first), fLastKey(last), fCallback(callback) {}
		std::map<unsigned,typename SubProcedure::SP> createSubProcedures(std::vector<Bin> const & entries) override final;
	};


	template <typename tKey>
	class ProcedureReadRecordsByKeysT : public ProcedureT<tKey>
	{
	public:
		typedef BinT<tKey> Bin;
		typedef RecordT<tKey> Record;
		typedef SubProcedureT<tKey> SubProcedure;
		typedef SubProcedureReadRecordsByKeysT<tKey> SubProcedureReadRecordsByKeys;
	private:
		std::vector<Record*> fRecords;
		typename Record::tReadByKeyFunction fCallback;
		void allSubProceduresWereDeleted() override final { this->announceAsCompleted(); }
	public:
		ProcedureReadRecordsByKeysT
		(SchedulerT<tKey>* scheduler, unsigned priority, typename Record::tReadByKeyFunction callback, std::vector<Record*> const & records)
		: ProcedureT<tKey>(scheduler, priority), fRecords(records), fCallback(callback) {}
		std::map<unsigned,typename SubProcedure::SP> createSubProcedures(std::vector<Bin> const & entries) override final;
	};


	template <typename tKey>
	class ProcedureUpdateRecordsByKeysT : public ProcedureT<tKey>
	{
	public:
		typedef BinT<tKey> Bin;
		typedef RecordT<tKey> Record;
		typedef SubProcedureT<tKey> SubProcedure;
		typedef SubProcedureUpdateRecordsByKeysT<tKey> SubProcedureUpdateRecordsByKeys;
	private:
		std::vector<Record*> fRecords;
		typename Record::tUpdateByKeyFunction fCallback;
		void allSubProceduresWereDeleted() override final { this->announceAsCompleted(); }
	public:
		ProcedureUpdateRecordsByKeysT
		(SchedulerT<tKey>* scheduler, unsigned priority, typename Record::tUpdateByKeyFunction callback, std::vector<Record*> const & records)
		: ProcedureT<tKey>(scheduler, priority), fRecords(records), fCallback(callback) {}
		std::map<unsigned,typename SubProcedure::SP> createSubProcedures(std::vector<Bin> const & entries) override final;
	};

}

#endif /* FLATDB_PROCEDURE_HPP_ */
