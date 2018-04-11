#ifndef CLUNET_SIMULATION_DATA_GENERATOR
#define CLUNET_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
//#include <string>
#include "ClunetPacket.h"

class ClunetAnalyzerSettings;

class ClunetSimulationDataGenerator
{
public:
	ClunetSimulationDataGenerator();
	~ClunetSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, ClunetAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	ClunetAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	U32 mSamplesPerPeriod;

protected:
	ClunetPacket packet;
	void CreateClunetPacket();

	SimulationChannelDescriptor mBusSimulationData;

};
#endif //CLUNET_SIMULATION_DATA_GENERATOR