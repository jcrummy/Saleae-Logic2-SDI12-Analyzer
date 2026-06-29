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
    bool mShowBreak;
    U32 mTimingTolerance; // SDI-12 spec allows +/-400 us timing tolerance.

  protected:
    AnalyzerSettingInterfaceChannel mInputChannelInterface;
    AnalyzerSettingInterfaceInteger mBitRateInterface;
    AnalyzerSettingInterfaceBool mShowBreakInterface;
    AnalyzerSettingInterfaceInteger mTimingToleranceInterface;
};

#endif // SDI12_ANALYZER_SETTINGS
