#include "ClunetAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


ClunetAnalyzerSettings::ClunetAnalyzerSettings()
	: mInputChannel(UNDEFINED_CHANNEL),
	mProtocolVersion(2.0),
	mPeriodUsec(64),
	mInverted(false)
{
	mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
	mInputChannelInterface->SetTitleAndTooltip("Data bus:", "Specify the data bus channel");
	mInputChannelInterface->SetChannel(mInputChannel);

	mProtocolVersionInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mProtocolVersionInterface->SetTitleAndTooltip("Version:", "Specify the protocol version");
	mProtocolVersionInterface->AddNumber(1.0, "1.0 - Original by Cluster", "Original CLUNET by Cluster");
	mProtocolVersionInterface->AddNumber(1.1, "1.1 - Modified w/o 10T init", "Modified CLUNET without 10T init pulse");
	mProtocolVersionInterface->AddNumber(2.0, "2.0 - NRZ coding with bitstuff", "NRZ coding with 5 bits bitstuffing");
	mProtocolVersionInterface->SetNumber(mProtocolVersion);

	mPeriodUsecInterface.reset(new AnalyzerSettingInterfaceInteger());
	mPeriodUsecInterface->SetTitleAndTooltip("Period T, us:", "Specify the period in microseconds");
	mPeriodUsecInterface->SetMax(6000000);
	mPeriodUsecInterface->SetMin(1);
	mPeriodUsecInterface->SetInteger(mPeriodUsec);

	mInvertedInterface.reset(new AnalyzerSettingInterfaceBool());
	mInvertedInterface->SetTitleAndTooltip("Inverted signal:", "Check if using transistor output");
	mInvertedInterface->SetValue(mInverted);

	mSimulateWrongChecksumInterface.reset(new AnalyzerSettingInterfaceBool());
	mSimulateWrongChecksumInterface->SetTitleAndTooltip("Simulate wrong checksum:", "Check if you want to simulate wrong checksum in packets");
	mSimulateWrongChecksumInterface->SetValue(mSimulateWrongChecksum);

	AddInterface(mInputChannelInterface.get());
	AddInterface(mProtocolVersionInterface.get());
	AddInterface(mPeriodUsecInterface.get());
	AddInterface(mInvertedInterface.get());
	AddInterface(mSimulateWrongChecksumInterface.get());

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "text", "txt");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();
	AddChannel(mInputChannel, "Data bus", false);
}

ClunetAnalyzerSettings::~ClunetAnalyzerSettings()
{
}

bool ClunetAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mProtocolVersion = mProtocolVersionInterface->GetNumber();
	mPeriodUsec = mPeriodUsecInterface->GetInteger();
	mInverted = mInvertedInterface->GetValue();
	mSimulateWrongChecksum = mSimulateWrongChecksumInterface->GetValue();

	ClearChannels();
	AddChannel(mInputChannel, "Data bus", true);

	return true;
}

void ClunetAnalyzerSettings::UpdateInterfacesFromSettings() {
	mInputChannelInterface->SetChannel(mInputChannel);
	mProtocolVersionInterface->SetNumber(mProtocolVersion);
	mPeriodUsecInterface->SetInteger(mPeriodUsec);
	mInvertedInterface->SetValue(mInverted);
	mSimulateWrongChecksumInterface->SetValue(mSimulateWrongChecksum);
}

void ClunetAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString(settings);

	text_archive >> mInputChannel;
	text_archive >> mProtocolVersion;
	text_archive >> mPeriodUsec;
	text_archive >> mInverted;
	text_archive >> mSimulateWrongChecksum;

	ClearChannels();
	AddChannel(mInputChannel, "Data bus", true);

	UpdateInterfacesFromSettings();
}

const char* ClunetAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mProtocolVersion;
	text_archive << mPeriodUsec;
	text_archive << mInverted;
	text_archive << mSimulateWrongChecksum;

	return SetReturnString(text_archive.GetString());
}
