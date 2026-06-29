#include "SDI12AnalyzerSettings.h"
#include <AnalyzerHelpers.h>


SDI12AnalyzerSettings::SDI12AnalyzerSettings()
    : mInputChannel( UNDEFINED_CHANNEL ), mBitRate( 1200 ), mShowBreak( false ), mTimingTolerance( 400 ), mInputChannelInterface(),
      mBitRateInterface()
{
    mInputChannelInterface.SetTitleAndTooltip( "Serial", "Standard SDI-12" );
    mInputChannelInterface.SetChannel( mInputChannel );

    mBitRateInterface.SetTitleAndTooltip( "Bit Rate (Bits/S)", "Specify the bit rate in bits per second." );
    mBitRateInterface.SetMax( 1200 );
    mBitRateInterface.SetMin( 1200 );
    mBitRateInterface.SetInteger( mBitRate );

    mShowBreakInterface.SetTitleAndTooltip( "Show break", "Check if you want the break explicitly shown." );
    mShowBreakInterface.SetValue( mShowBreak );

    mTimingToleranceInterface.SetTitleAndTooltip(
        "Timing tolerance (us)",
        "Extra timing margin in microseconds applied to break/marking detection. SDI-12 allows +/-400 us; Increase to assist in debugging timing issues." );
    mTimingToleranceInterface.SetMax( 2000 );
    mTimingToleranceInterface.SetMin( 0 );
    mTimingToleranceInterface.SetInteger( mTimingTolerance );

    AddInterface( &mInputChannelInterface );
    AddInterface( &mBitRateInterface );
	AddInterface( &mShowBreakInterface );
	AddInterface( &mTimingToleranceInterface );

    AddExportOption( 0, "Export as text/csv file" );
    AddExportExtension( 0, "text", "txt" );
    AddExportExtension( 0, "csv", "csv" );

    ClearChannels();
    AddChannel( mInputChannel, "Serial", false );
}

SDI12AnalyzerSettings::~SDI12AnalyzerSettings()
{
}

bool SDI12AnalyzerSettings::SetSettingsFromInterfaces()
{
    mInputChannel = mInputChannelInterface.GetChannel();
    mBitRate = mBitRateInterface.GetInteger();
	mShowBreak = mShowBreakInterface.GetValue();
	mTimingTolerance = mTimingToleranceInterface.GetInteger();

    ClearChannels();
    AddChannel( mInputChannel, "SDI-12", true );

    return true;
}

void SDI12AnalyzerSettings::UpdateInterfacesFromSettings()
{
    mInputChannelInterface.SetChannel( mInputChannel );
    mBitRateInterface.SetInteger( mBitRate );
	mShowBreakInterface.SetValue( mShowBreak );
	mTimingToleranceInterface.SetInteger( mTimingTolerance );
}

void SDI12AnalyzerSettings::LoadSettings( const char* settings )
{
    SimpleArchive text_archive;
    text_archive.SetString( settings );

    text_archive >> mInputChannel;
    text_archive >> mBitRate;

    mShowBreak = false;
    mTimingTolerance = 400;
    text_archive >> mShowBreak;
    text_archive >> mTimingTolerance;

    ClearChannels();
    AddChannel( mInputChannel, "SDI-12", true );

    UpdateInterfacesFromSettings();
}

const char* SDI12AnalyzerSettings::SaveSettings()
{
    SimpleArchive text_archive;

    text_archive << mInputChannel;
    text_archive << mBitRate;
    text_archive << mShowBreak;
    text_archive << mTimingTolerance;

    return SetReturnString( text_archive.GetString() );
}
