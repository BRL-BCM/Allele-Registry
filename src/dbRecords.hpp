#ifndef DBRECORDS_HPP_
#define DBRECORDS_HPP_

#include "keyValueDb.hpp"

struct Record_def_caId
{
	unsigned rsId = 0;
	unsigned position = 0;
	unsigned length = 0;
	unsigned insertion = 0;
	unsigned caId = 0;
	inline keyValueDb::Buffer getKey() const
	{
		keyValueDb::Buffer buf(12);
		buf.writeBytes(rsId, 3);
		buf.writeBytes(position, 4);
		buf.writeBytes(length, 2);
		buf.writeBytes(insertion, 3);
		return buf;
	};
	inline keyValueDb::Buffer getValue() const
	{
		keyValueDb::Buffer buf(4);
		buf.writeBytes(caId, 4);
		return buf;
	}
	inline void saveValue(keyValueDb::Buffer & data)
	{
		data.readBytes(caId, 4);
	}
};

struct Record_caId_def
{
	unsigned rsId = 0;
	unsigned position = 0;
	unsigned length = 0;
	unsigned insertion = 0;
	unsigned caId = 0;
	inline keyValueDb::Buffer getKey() const
	{
		keyValueDb::Buffer buf(4);
		buf.writeBytes(caId, 4);
		return buf;
	}
	inline keyValueDb::Buffer getValue() const
	{
		keyValueDb::Buffer buf(12);
		buf.writeBytes(rsId, 3);
		buf.writeBytes(position, 4);
		buf.writeBytes(length, 2);
		buf.writeBytes(insertion, 3);
		return buf;
	};

	inline void saveValue(keyValueDb::Buffer & buf)
	{
		buf.readBytes(rsId, 3);
		buf.readBytes(position, 4);
		buf.readBytes(length, 2);
		buf.readBytes(insertion, 3);
	}
};

struct Record_seqId_seq
{
	unsigned seqId = 0;
	std::string seq = "";
	inline keyValueDb::Buffer getKey() const
	{
		keyValueDb::Buffer buf(3);
		buf.writeBytes(seqId, 3);
		return buf;
	};
	inline keyValueDb::Buffer getValue() const
	{
		keyValueDb::Buffer buf(seq.size());
		buf.writeString(seq);
		return buf;
	}
	inline void saveValue(keyValueDb::Buffer & data)
	{
		data.readString(seq, data.size-data.position);
	}
};

struct Record_seq_seqId
{
	unsigned seqId = 0;
	std::string seq = "";
	inline keyValueDb::Buffer getKey() const
	{
		keyValueDb::Buffer buf(seq.size());
		buf.writeString(seq);
		return buf;
	};
	inline keyValueDb::Buffer getValue() const
	{
		keyValueDb::Buffer buf(3);
		buf.writeBytes(seqId, 3);
		return buf;
	}
	inline void saveValue(keyValueDb::Buffer & data)
	{
		data.readBytes(seqId, 3);
	}
};

#endif /* DBRECORDS_HPP_ */
