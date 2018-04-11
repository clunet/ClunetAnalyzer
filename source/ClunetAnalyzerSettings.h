#ifndef CLUNET_ANALYZER_SETTINGS
#define CLUNET_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class ClunetAnalyzerSettings : public AnalyzerSettings
{
public:
	ClunetAnalyzerSettings();
	virtual ~ClunetAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel	mInputChannel;
	double	mProtocolVersion;
	U32		mPeriodUsec;
	bool	mInverted;
	bool	mSimulateWrongChecksum;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mProtocolVersionInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mPeriodUsecInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		mInvertedInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >		mSimulateWrongChecksumInterface;
};

#endif //CLUNET_ANALYZER_SETTINGS
