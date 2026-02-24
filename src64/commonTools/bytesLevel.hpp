#ifndef BYTESLEVEL_HPP_
#define BYTESLEVEL_HPP_

#include <cstdint>
#include <boost/crc.hpp>

// ======================================= SAVE SHIFTING

template<typename tUnsigned>
inline tUnsigned shiftRight(tUnsigned v, unsigned bits)
{
	if (bits < sizeof(tUnsigned)*8) return (v >> bits);
	return static_cast<tUnsigned>(0);
}
template<unsigned bits, typename tUnsigned>
inline tUnsigned shiftRight(tUnsigned v)
{
	if (bits < sizeof(tUnsigned)*8) return (v >> bits);
	return static_cast<tUnsigned>(0);
}

template<typename tUnsigned>
inline tUnsigned shiftLeft(tUnsigned v, unsigned bits)
{
	if (bits < sizeof(tUnsigned)*8) return (v << bits);
	return static_cast<tUnsigned>(0);
}
template<unsigned bits, typename tUnsigned>
inline tUnsigned shiftLeft(tUnsigned v)
{
	if (bits < sizeof(tUnsigned)*8) return (v << bits);
	return static_cast<tUnsigned>(0);
}

template<typename tUnsigned>
inline tUnsigned shiftRightInPlace(tUnsigned & v, unsigned bits)
{
	if (bits < sizeof(tUnsigned)*8) v >>= bits;
	else v = static_cast<tUnsigned>(0);
	return v;
}
template<unsigned bits, typename tUnsigned>
inline tUnsigned shiftRightInPlace(tUnsigned & v)
{
	if (bits < sizeof(tUnsigned)*8) v >>= bits;
	else v = static_cast<tUnsigned>(0);
	return v;
}

template<typename tUnsigned>
inline tUnsigned shiftLeftInPlace(tUnsigned & v, unsigned bits)
{
	if (bits < sizeof(tUnsigned)*8) v <<= bits;
	else v = static_cast<tUnsigned>(0);
	return v;
}
template<unsigned bits, typename tUnsigned>
inline tUnsigned shiftLeftInPlace(tUnsigned & v)
{
	if (bits < sizeof(tUnsigned)*8) v <<= bits;
	else v = static_cast<tUnsigned>(0);
	return v;
}


// ===================================== CRC32

inline unsigned CRC32(uint8_t const * buf, unsigned size)
{
	boost::crc_32_type result;
	result.process_bytes(buf, size);
	return result.checksum();
}


// ===================================== READ/WRITE UNSIGNED VALUES ON CONSTANT NUMBER OF BYTES

template<typename targetType>
targetType readUnsignedInteger(uint8_t const *& ptr, unsigned bytesCount)
{
	targetType value = 0;
	for ( ;  bytesCount > 0;  --bytesCount ) {
		shiftLeftInPlace<8>(value);
		value += *ptr;
		++ptr;
	}
	return value;
}
template<unsigned bytesCount, typename targetType>
targetType readUnsignedInteger(uint8_t const *& ptr)
{
	targetType value = 0;
	for ( unsigned bytesCount2 = bytesCount ;  bytesCount2 > 0;  --bytesCount2 ) {
		shiftLeftInPlace<8>(value);
		value += *ptr;
		++ptr;
	}
	return value;
}

// save "bytesCount" the least significant bytes, the rest is ignored
template<typename sourceType>
void writeUnsignedInteger(uint8_t *& ptr, sourceType value, unsigned bytesCount)
{
	while ( bytesCount > 0 ) {
		--bytesCount;
		*ptr = (shiftRight(value,(8*bytesCount))) % 256;
		++ptr;
	}
}
template<unsigned bytesCount, typename sourceType>
void writeUnsignedInteger(uint8_t *& ptr, sourceType value)
{
	for (unsigned i = 1; i <= bytesCount; ++i ) {
		*ptr = (shiftRight(value,8*(bytesCount-i))) % 256;
		++ptr;
	}
}

// ===================================== OPERATIONS ON BITS

template<unsigned bit, typename tUnsigned>
inline void setBits(tUnsigned & v)
{
	v |= shiftLeft<sizeof(tUnsigned)*8-1-bit>(static_cast<tUnsigned>(1));
}
template<unsigned bit1, unsigned bit2, typename tUnsigned>
inline void setBits(tUnsigned & v)
{
	setBits<bit1>(v);
	setBits<bit2>(v);
}
template<unsigned bit1, unsigned bit2, unsigned bit3, typename tUnsigned>
inline void setBits(tUnsigned & v)
{
	setBits<bit1>(v);
	setBits<bit2>(v);
	setBits<bit3>(v);
}
template<unsigned bit1, unsigned bit2, unsigned bit3, unsigned bit4, typename tUnsigned>
inline void setBits(tUnsigned & v)
{
	setBits<bit1>(v);
	setBits<bit2>(v);
	setBits<bit3>(v);
	setBits<bit4>(v);
}

template<unsigned bit, typename tUnsigned>
inline bool checkBits(tUnsigned const & v)
{
	return ( v & shiftLeft<sizeof(tUnsigned)*8-1-bit>(static_cast<tUnsigned>(1)) );
}


// it assumes that given bits are set to 0
template<unsigned bit, unsigned length, typename tUnsigned>
inline void setValue(tUnsigned & v, unsigned value)
{
	tUnsigned p = static_cast<tUnsigned>(value % shiftLeft<length>(1u));
	shiftLeftInPlace<sizeof(tUnsigned)*8-length-bit>(p);
	v |= p;
}

template<unsigned bit, unsigned length, typename tUnsigned>
inline unsigned getValue(tUnsigned const & v)
{
	return shiftRight<sizeof(tUnsigned)*8-length>( shiftLeft<bit>(v) );
}

// ===================================== READ/WRITE UNSIGNED VALUES ON VARIABLE NUMBER OF BYTES

template<unsigned firstChunkInBytes = 1, unsigned nextChunksInBytes = 1, typename sourceType>
unsigned lengthUnsignedIntVarSize(sourceType value)
{
	unsigned const firstChunkInBits = 8*firstChunkInBytes-1;
	shiftRightInPlace<firstChunkInBits>(value);
	if (value == 0) return firstChunkInBytes;
	return (firstChunkInBytes + lengthUnsignedIntVarSize<nextChunksInBytes,nextChunksInBytes,sourceType>(value));
}

// save unsigned int on variable count of bytes
template<unsigned firstChunkInBytes = 1, unsigned nextChunksInBytes = 1, typename sourceType>
void writeUnsignedIntVarSize(uint8_t *& ptr, sourceType value)
{
	if (firstChunkInBytes == 0) {
		writeUnsignedIntVarSize<nextChunksInBytes,nextChunksInBytes>(ptr,value);
		return;
	}
	if (nextChunksInBytes == 0) {
		writeUnsignedInteger<firstChunkInBytes>(ptr,value);
		return;
	}
	unsigned const firstChunkInBits = 8*firstChunkInBytes-1;
	unsigned const nextChunksInBits = 8*nextChunksInBytes-1;
	if ( shiftRight<firstChunkInBits>(value) == 0 ) {
		// write first chunk
		writeUnsignedInteger<firstChunkInBytes>(ptr, value);
		return;
	}
	sourceType const v2 = shiftRight<nextChunksInBits>(value);
	writeUnsignedIntVarSize<firstChunkInBytes,nextChunksInBytes>(ptr, v2);
	// set extension bit
	if (shiftRight<firstChunkInBits>(v2) == 0) {
		*(ptr-firstChunkInBytes) |= static_cast<uint8_t>(0x80);
	} else {
		*(ptr-nextChunksInBytes) |= static_cast<uint8_t>(0x80);
	}
	// write next chunk
	writeUnsignedInteger<nextChunksInBytes>(ptr, value - shiftLeft<nextChunksInBits>(v2));
}

// load unsigned int from field with variable count of bytes
template<unsigned firstChunkInBytes = 1, unsigned nextChunksInBytes = 1, typename sourceType>
sourceType readUnsignedIntVarSize(uint8_t const *& ptr)
{
	if (firstChunkInBytes == 0) {
		return readUnsignedIntVarSize<nextChunksInBytes,nextChunksInBytes,sourceType>(ptr);
	}
	if (nextChunksInBytes == 0) {
		return readUnsignedInteger<firstChunkInBytes,sourceType>(ptr);
	}
	bool extension = *ptr & static_cast<uint8_t>(0x80);
	sourceType value = readUnsignedInteger<firstChunkInBytes,sourceType>(ptr);
	if (extension) {
		unsigned const firstChunkInBits = 8*firstChunkInBytes-1;
		unsigned const nextChunksInBits = 8*nextChunksInBytes-1;
		value -= static_cast<sourceType>(0x01) << firstChunkInBits;
		while (extension) {
			extension = *ptr & static_cast<uint8_t>(0x80);
			sourceType v2 = readUnsignedInteger<nextChunksInBytes,sourceType>(ptr);
			if (extension) v2 -= static_cast<sourceType>(0x01) << nextChunksInBits;
			shiftLeftInPlace<nextChunksInBits>(value);
			value += v2;
		}
	}
	return value;
}



#endif /* BYTESLEVEL_HPP_ */
