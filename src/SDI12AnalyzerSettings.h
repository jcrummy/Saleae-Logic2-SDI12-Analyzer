#ifndef SDI12_ANALYZER_SETTINGS
#define SDI12_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class SDI12AnalyzerSettings : public AnalyzerSettings
{
public:
	SDI12AnalyzerSettings();
	virtual ~SDI12AnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	U32 mBitRate;

protected:
	AnalyzerSettingInterfaceChannel	mInputChannelInterface;
	AnalyzerSettingInterfaceInteger	mBitRateInterface;
};

#endif //SDI12_ANALYZER_SETTINGS
