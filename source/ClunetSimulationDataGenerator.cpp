#include "ClunetSimulationDataGenerator.h"
#include "ClunetAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

ClunetSimulationDataGenerator::ClunetSimulationDataGenerator() {}
ClunetSimulationDataGenerator::~ClunetSimulationDataGenerator() {}

void ClunetSimulationDataGenerator::Initialize(U32 simulation_sample_rate, ClunetAnalyzerSettings* settings) {
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;
	mSamplesPerPeriod = mSimulationSampleRateHz * mSettings->mPeriodUsec / 1000000;

	mBusSimulationData.SetChannel(mSettings->mInputChannel);
	mBusSimulationData.SetSampleRate(simulation_sample_rate);
	mBusSimulationData.SetInitialBitState(mSettings->mInverted ? BIT_LOW : BIT_HIGH);
}

U32 ClunetSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mBusSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		CreateClunetPacket();
	}

	*simulation_channel = &mBusSimulationData;
	return 1;
}

void ClunetSimulationDataGenerator::CreateClunetPacket()
{
	// Fill packet with new data
	packet.FillRandom(mSettings->mSimulateWrongChecksum);

	// Simulate start 10T pulse (only for 1.0)
	if (mSettings->mProtocolVersion == 1.0) {
		mBusSimulationData.Advance(mSamplesPerPeriod * 2);
		mBusSimulationData.Transition();
		mBusSimulationData.Advance(mSamplesPerPeriod * 10);
		mBusSimulationData.Transition();
	}
	// Simulate interframe window for others
	else
	{
		mBusSimulationData.Advance(mSamplesPerPeriod * 10);
	}

	U8 max_idx = packet.GetMaxIndex();
	U8 idx = 0;

	if (mSettings->mProtocolVersion == 2.0) {

		U8 bitstuff = 0;
		U8 mask = 0x04;

		do {

			U8 byte = packet[idx];

			do {

				if (!bitstuff) {

					bitstuff = 4;
					mBusSimulationData.Transition();

				}
				else {

					if (!(byte & mask) == (mBusSimulationData.GetCurrentBitState() == (mSettings->mInverted ? BIT_HIGH : BIT_LOW))) {

						bitstuff = 4;
						mBusSimulationData.Transition();

					}

					else
					{
						--bitstuff;
					}

					mask >>= 1;
				
				}

				mBusSimulationData.Advance(mSamplesPerPeriod);

			} while (mask);

			mask = 0x80;

		} while (++idx <= max_idx);

		if (mBusSimulationData.GetCurrentBitState() == (mSettings->mInverted ? BIT_LOW : BIT_HIGH)) {

			mBusSimulationData.Transition();
			mBusSimulationData.Advance(mSamplesPerPeriod);

		}

		mBusSimulationData.Transition();

	}
	else
	{
		U8 mask = 0x02;

		do {

			U8 byte = packet[idx];

			do {

				mBusSimulationData.Advance(mSamplesPerPeriod);
				mBusSimulationData.Transition();
				mBusSimulationData.Advance((byte & mask) ? mSamplesPerPeriod * 3 : mSamplesPerPeriod);
				mBusSimulationData.Transition();

			} while (mask >>= 1);

			mask = 0x80;

		} while (++idx <= max_idx);
	}
}
