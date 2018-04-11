#include "ClunetAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "ClunetAnalyzer.h"
#include "ClunetAnalyzerSettings.h"
#include "ClunetPacket.h"
#include <iostream>
#include <fstream>

ClunetAnalyzerResults::ClunetAnalyzerResults(ClunetAnalyzer* analyzer, ClunetAnalyzerSettings* settings)
	: AnalyzerResults(),
	mSettings(settings),
	mAnalyzer(analyzer)
{
}

ClunetAnalyzerResults::~ClunetAnalyzerResults()
{
}

void ClunetAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame frame = GetFrame(frame_index);

	char number_str[128];
	char buf[128];

	if ((display_base == DisplayBase::ASCII || display_base == DisplayBase::AsciiHex) && frame.mType != ClunetPacket::Data)
		display_base = (frame.mType == ClunetPacket::CRC) ? DisplayBase::Hexadecimal : DisplayBase::Decimal;

	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);

	switch (frame.mType)
	{
	case ClunetPacket::Priority:
		AddResultString("PR");
		snprintf(buf, 128, "PRIO: %s", number_str);
		AddResultString(buf);
		snprintf(buf, 128, "Priority: %s", number_str);
		AddResultString(buf);
		break;
	case ClunetPacket::DstAddress:
		AddResultString("DA");
		snprintf(buf, 128, "DST: %s", number_str);
		AddResultString(buf);
		snprintf(buf, 128, "Destination address: %s", number_str);
		AddResultString(buf);
		break;
	case ClunetPacket::SrcAddress:
		AddResultString("SA");
		snprintf(buf, 128, "SRC: %s", number_str);
		AddResultString(buf);
		snprintf(buf, 128, "Source address: %s", number_str);
		AddResultString(buf);
		break;
	case ClunetPacket::Command:
		AddResultString("CM");
		snprintf(buf, 128, "CM: %s", number_str);
		AddResultString(buf);
		snprintf(buf, 128, "Command: %s", number_str);
		AddResultString(buf);
		break;
	case ClunetPacket::DataSize:
		if (frame.HasFlag(ClunetPacket::DataSizeTooBig)) {
			AddResultString("!SZ");
			snprintf(buf, 128, "!SZ: %s", number_str);
			AddResultString(buf);
			snprintf(buf, 128, "!Data size (error: too big): %s", number_str);
			AddResultString(buf);
		}
		else
		{
			AddResultString("SZ");
			snprintf(buf, 128, "SZ: %s", number_str);
			AddResultString(buf);
			snprintf(buf, 128, "Data size: %s", number_str);
			AddResultString(buf);
		}
		break;
	case ClunetPacket::CRC:
		if (frame.HasFlag(DISPLAY_AS_ERROR_FLAG)) {
			AddResultString("!CS");
			snprintf(buf, 128, "!CS: %s", number_str);
			AddResultString(buf);
			snprintf(buf, 128, "!Checksum is WRONG: %s", number_str);
			AddResultString(buf);
		}
		else {
			AddResultString("CS");
			snprintf(buf, 128, "CS: %s", number_str);
			AddResultString(buf);
			snprintf(buf, 128, "Checksum is OK: %s", number_str);
			AddResultString(buf);
		}
		break;
	case ClunetPacket::Data:
		AddResultString("D");
		snprintf(buf, 128, "D: %s", number_str);
		AddResultString(buf);
		snprintf(buf, 128, "D[%lld]: %s", frame.mData2, number_str);
		AddResultString(buf);
		snprintf(buf, 128, "Data byte %lld: %s", frame.mData2, number_str);
		AddResultString(buf);
		break;
	default:
		AddResultString("!");
		AddResultString("!: ", number_str);
		AddResultString("!Unknown byte: ", number_str);
		break;
	}

}

void ClunetAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream(file, std::ios::out);

	file_stream << "Protocol name: " << GetAnalyzerName() << std::endl;
	file_stream << "Protocol version: " << mSettings->mProtocolVersion << std::endl;
	file_stream << std::endl;

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	U64 num_packets = GetNumPackets();
	U64 num_frames = GetNumFrames();
	U64 current_frame = 0;

	file_stream << "Packets in export file: " << num_packets << std::endl;
	file_stream << std::endl;

	for (U64 i = 0; i < num_packets; i++)
	{

		U64 last_frame = 0;

		GetFramesContainedInPacket(i, &current_frame, &last_frame);

		file_stream << "Packet #" << i + 1 << " (" << (last_frame - current_frame + 1) << " frames)" << std::endl;
		file_stream << std::endl;


		char time_str[128];
		Frame frame = GetFrame(current_frame);
		S64 start_frame = frame.mStartingSampleInclusive;
		frame = GetFrame(last_frame);

		AnalyzerHelpers::GetTimeString((frame.mEndingSampleInclusive - start_frame) * 1000, trigger_sample, sample_rate, time_str, 128);
		file_stream << "Length: " << time_str << " ms" << std::endl;
		file_stream << std::endl;

		do
		{

			frame = GetFrame(current_frame);

			file_stream << ClunetPacket::FrameTypeToString(frame.mType);

			if (frame.mType == ClunetPacket::Data)
				file_stream << " #" << frame.mData2;

			file_stream << ": " << frame.mData1 << " (0x" << std::hex << frame.mData1 << std::dec << ")" << std::endl;

			if (frame.mType == ClunetPacket::DataSize)
				file_stream << std::endl;

			if (UpdateExportProgressAndCheckForCancel(current_frame, num_frames) == true)
			{
				file_stream.close();
				return;
			}

		} while (++current_frame <= last_frame);

		file_stream << std::endl;

	}

	UpdateExportProgressAndCheckForCancel(num_frames - 1, num_frames);

	file_stream.close();

}

void ClunetAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
#ifdef SUPPORTS_PROTOCOL_SEARCH
	Frame frame = GetFrame(frame_index);
	ClearTabularText();

	char number_str[128];
	char buf[128];

	if ((display_base == DisplayBase::ASCII || display_base == DisplayBase::AsciiHex) && frame.mType != ClunetPacket::Data)
		display_base = (frame.mType == ClunetPacket::CRC) ? DisplayBase::Hexadecimal : DisplayBase::Decimal;

	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, 128);

	switch (frame.mType)
	{
	case ClunetPacket::Priority:
		snprintf(buf, 128, "Priority: %s", number_str);
		AddTabularText(buf);
		break;
	case ClunetPacket::DstAddress:
		snprintf(buf, 128, "Destination address: %s", number_str);
		AddTabularText(buf);
		break;
	case ClunetPacket::SrcAddress:
		snprintf(buf, 128, "Source address: %s", number_str);
		AddTabularText(buf);
		break;
	case ClunetPacket::Command:
		snprintf(buf, 128, "Command: %s", number_str);
		AddTabularText(buf);
		break;
	case ClunetPacket::DataSize:
		if (frame.HasFlag(ClunetPacket::DataSizeTooBig)) {
			snprintf(buf, 128, "!Data size (error: too big): %s", number_str);
			AddTabularText(buf);
		}
		else
		{
			snprintf(buf, 128, "Data size: %s", number_str);
			AddTabularText(buf);
		}
		break;
	case ClunetPacket::CRC:
		if (frame.HasFlag(ClunetPacket::WrongCRC))
			snprintf(buf, 128, "!Checksum WRONG: %s", number_str);
		else
			snprintf(buf, 128, "Checksum OK: %s", number_str);
		AddTabularText(buf);
		break;
	case ClunetPacket::Data:
		snprintf(buf, 128, "Data #%lld: %s", frame.mData2, number_str);
		AddTabularText(buf);
		break;
	default:
		AddTabularText("!Unknown byte: ", number_str);
		break;
	}

#endif
}

void ClunetAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	//not supported

}

void ClunetAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	//not supported
}