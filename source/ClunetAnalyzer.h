#ifndef CLUNET_ANALYZER_H
#define CLUNET_ANALYZER_H

#include <Analyzer.h>
#include "ClunetAnalyzerResults.h"
#include "ClunetSimulationDataGenerator.h"

class ClunetAnalyzerSettings;
class ANALYZER_EXPORT ClunetAnalyzer : public Analyzer2
{
public:
	ClunetAnalyzer();
	virtual ~ClunetAnalyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< ClunetAnalyzerSettings > mSettings;
	std::auto_ptr< ClunetAnalyzerResults > mResults;
	AnalyzerChannelData* mBus;

	ClunetSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	// Clunet analysis vars:
	ClunetPacket mPacket;
	U64 mStartFrameSample;
	U8 mCurrentByte;
	U32 mByteIndex;
	U32 mBitIndex;
	U32 mSamplesPerPeriod;
	void AddFrameToResult(U64 sub_frames = 0);
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //CLUNET_ANALYZER_H
