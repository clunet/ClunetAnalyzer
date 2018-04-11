#include "ClunetAnalyzer.h"
#include "ClunetAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

ClunetAnalyzer::ClunetAnalyzer()
	: Analyzer2(),
	mSettings(new ClunetAnalyzerSettings()),
	mSimulationInitilized(false)
{
	SetAnalyzerSettings(mSettings.get());
}

ClunetAnalyzer::~ClunetAnalyzer()
{
	KillThread();
}

void ClunetAnalyzer::SetupResults()
{
	mResults.reset(new ClunetAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());
	mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

void ClunetAnalyzer::WorkerThread()
{
	mSamplesPerPeriod = GetSampleRate() * mSettings->mPeriodUsec / 1000000;

	mBus = GetAnalyzerChannelData(mSettings->mInputChannel);

	U64 start_sample = 0;

	// find init state
	if ((mSettings->mInverted == (mSettings->mProtocolVersion == 1.0)) == (mBus->GetBitState() == BIT_LOW)) {
		mBus->AdvanceToNextEdge();
		start_sample = mBus->GetSampleNumber();
	}

	mCurrentByte = 0;
	mStartFrameSample = start_sample;
	mBitIndex = (mSettings->mProtocolVersion == 2.0) ? 5 : 6;
	mByteIndex = 0;

	bool bitstuff = true;

	for (;;)
	{
		// go to next edge
		mBus->AdvanceToNextEdge();

		// corrected new bitstate
		bool bit_state = (mBus->GetBitState() == BIT_LOW) == (mSettings->mInverted);

		// time of pulse in samples
		U64 pulse_samples = mBus->GetSampleNumber() - start_sample;

		// time of pulse in T periods
		U32 pulse_periods = (pulse_samples + mSamplesPerPeriod / 2) / mSamplesPerPeriod;

		if (!pulse_periods)
			continue;

		// detecting reset condition
		if (pulse_periods >= 7 && ((mSettings->mProtocolVersion == 1.0) == bit_state)) {

			mCurrentByte = 0;
			mStartFrameSample = mBus->GetSampleNumber();
			mBitIndex = (mSettings->mProtocolVersion == 2.0) ? 5 : 6;
			mByteIndex = 0;
			bitstuff = true;
			mResults->AddMarker(start_sample + pulse_samples / 2, AnalyzerResults::Start, mSettings->mInputChannel);
			mResults->CancelPacketAndStartNewPacket();
		}

		// ordinary pulse of protocol version is 2.0
		else if (mSettings->mProtocolVersion == 2.0)
		{
			const U64 samples_in_bit = pulse_samples / pulse_periods;

			U32 cnt = 0;
			U64 mark_sample = start_sample + samples_in_bit / 2;

			do {

				if (cnt < 5) {

					if (!cnt && bitstuff)
						// mark bitstaff bit
						mResults->AddMarker(mark_sample, AnalyzerResults::X, mSettings->mInputChannel);

					else
						// mark data bits
						mResults->AddMarker(mark_sample, bit_state ? AnalyzerResults::One : AnalyzerResults::Zero, mSettings->mInputChannel);

				}
				else {
					// mark all errors bits
					mResults->AddMarker(mark_sample, AnalyzerResults::ErrorDot, mSettings->mInputChannel);
				}

				mark_sample += samples_in_bit;

			} while (++cnt < pulse_periods);

			const U8 bit_mask = 255 >> mBitIndex;

			if (bit_state)
				mCurrentByte |= bit_mask;
			else
				mCurrentByte &= ~bit_mask;

			mBitIndex += pulse_periods - bitstuff;

			if (mBitIndex & 8) {

				mBitIndex &= 7;
				AddFrameToResult(samples_in_bit * mBitIndex);
				mCurrentByte = bit_state ? 255 : 0;

			}

			bitstuff = (pulse_periods > 4);

		}


		// ordinary pulse, protocol versions is 1.x, low pulse
		else if (bit_state) {

			if (pulse_periods >= 2)
			{
				mResults->AddMarker(start_sample + pulse_samples / 2, AnalyzerResults::One, mSettings->mInputChannel);
				mCurrentByte |= (0x80 >> mBitIndex);
			}
			else
			{
				mResults->AddMarker(start_sample + pulse_samples / 2, AnalyzerResults::Zero, mSettings->mInputChannel);
			}

			if (++mBitIndex & 8) {

				mBitIndex = 0;
				AddFrameToResult();
				mCurrentByte = 0;

			}
		}

		mResults->CommitResults();
		start_sample = mBus->GetSampleNumber();

	}
}

bool ClunetAnalyzer::NeedsRerun()
{
	return false;
}

void ClunetAnalyzer::AddFrameToResult(U64 sub_frames)
{
	Frame frame;

	frame.mData1 = mCurrentByte;
	frame.mFlags = 0;
	frame.mStartingSampleInclusive = mStartFrameSample;

	mStartFrameSample = mBus->GetSampleNumber() - sub_frames;

	frame.mEndingSampleInclusive = mStartFrameSample - 1;

	mPacket[mByteIndex] = mCurrentByte;

	bool commit_packet = false;

	switch (mByteIndex)
	{
	case 0:

		frame.mType = ClunetPacket::Priority;
		break;

	case 1:

		frame.mType = (mSettings->mProtocolVersion == 1.0) ? ClunetPacket::SrcAddress : ClunetPacket::DstAddress;
		break;

	case 2:

		frame.mType = (mSettings->mProtocolVersion == 1.0) ? ClunetPacket::DstAddress : ClunetPacket::SrcAddress;
		break;

	case 3:

		frame.mType = ClunetPacket::Command;
		break;

	case 4:

		frame.mType = ClunetPacket::DataSize;
		break;

	default: // >= 5

		U32 crc_index = mPacket.GetMaxIndex();

		if (mByteIndex < crc_index) {

			frame.mType = ClunetPacket::Data;
			frame.mData2 = mByteIndex - 5;

		}

		else if (mByteIndex == crc_index) {

			frame.mType = ClunetPacket::CRC;

			if (mCurrentByte != mPacket.CalculateChecksum())
				frame.mFlags |= ClunetPacket::WrongCRC | DISPLAY_AS_ERROR_FLAG;

			else
				commit_packet = true;

		}

		else {

			frame.mType = ClunetPacket::Unknown;
			frame.mFlags |= DISPLAY_AS_WARNING_FLAG;

		}

	}

	mByteIndex++;

	mResults->AddFrame(frame);

	if (commit_packet)
		mResults->CommitPacketAndStartNewPacket();

	ReportProgress(frame.mEndingSampleInclusive);

}

U32 ClunetAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (mSimulationInitilized == false)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 ClunetAnalyzer::GetMinimumSampleRateHz()
{
	return 4000000 / mSettings->mPeriodUsec;
}

const char* ClunetAnalyzer::GetAnalyzerName() const
{
	return "Clunet";
}

const char* GetAnalyzerName()
{
	return "Clunet";
}

Analyzer* CreateAnalyzer()
{
	return new ClunetAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}