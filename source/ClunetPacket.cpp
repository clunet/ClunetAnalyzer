#include "ClunetPacket.h"

ClunetPacket::ClunetPacket()
{
	FillRandom();
}

U8 &ClunetPacket::operator[](U32 idx)
{
	if (idx < (U32)mDataSize + 5) {
		
		U8 *ptr = (U8 *)&mPriority;
		return ptr[idx];
	
	}

	return mChecksum;
}

U8 ClunetPacket::GetMaxIndex() const
{
	return mDataSize + 5;
}

U8 ClunetPacket::CalculateChecksum() const
{
	U8 crc = 0;
	U8 size = mDataSize + 4;
	U8 *ptr = (U8 *)&mDstAddress;

	do {

		crc = crc ^ *ptr;

		for (int i = 0; i < 8; i++)
		{
			if (crc & 0x01)
				crc = (crc >> 1) ^ 0x8C;
			else
				crc >>= 1;
		}

		ptr++;

	} while (--size);

	return crc;
}

void ClunetPacket::FillRandom(bool wrong_checksum)
{
	mPriority = rand();
	mDstAddress = rand();

	do
		mSrcAddress = rand();
	while (mSrcAddress == 0);

	mCommand = rand();
	mDataSize = rand() % (CLUNET_MAX_DATASIZE + 1);

	for (U8 idx = 0; idx < mDataSize; idx++)
		mData[idx] = rand();

	UpdateCRC(wrong_checksum);
}

void ClunetPacket::UpdateCRC(bool wrong)
{

	mChecksum = CalculateChecksum();

	if (wrong) {

		U8 crc;

		do
			crc = rand();
		while (crc == mChecksum);

		mChecksum = crc;

	}

}

const char * ClunetPacket::FrameTypeToString(U8 type)
{
	switch (type) {

	case Priority:
		return "Priority";

	case DstAddress:
		return "Destination address";

	case SrcAddress:
		return "Source address";

	case Command:
		return "Command";

	case DataSize:
		return "Data size";

	case Data:
		return "Data";

	case CRC:
		return "Checksum";

	}

	return "Unknown byte";
}
