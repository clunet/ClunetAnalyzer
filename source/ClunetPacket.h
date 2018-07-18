#pragma once
#include <AnalyzerTypes.h>


#define CLUNET_MAX_DATASIZE 250

class ClunetPacket
{

	U8 mPriority;
	U8 mDstAddress;
	U8 mSrcAddress;
	U8 mCommand;
	U8 mDataSize;
	U8 mData[CLUNET_MAX_DATASIZE];
	U8 mChecksum;

	void UpdateCRC(bool wrong = false);

public:

	enum FieldType
	{
		Unknown,
		Priority,
		DstAddress,
		SrcAddress,
		Command,
		DataSize,
		Data,
		CRC
	};

	enum ErrorFlag
	{
		DataSizeTooBig			= 0x01,
		WrongCRC				= 0x02,
		SrcAddressIsBroadcast	= 0x04
	};

	ClunetPacket();

	U8 &operator[](U32 idx);
	U8 CalculateChecksum() const;
	U8 GetMaxIndex() const;
	void FillRandom(bool wrong_checksum = false);
	static const char * FrameTypeToString(U8 type);
};

