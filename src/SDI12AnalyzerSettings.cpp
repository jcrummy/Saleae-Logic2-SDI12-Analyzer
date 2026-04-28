#include "SDI12AnalyzerSettings.h"
#include <AnalyzerHelpers.h>


SDI12AnalyzerSettings::SDI12AnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 1200 ),
	mInputChannelInterface(),
	mBitRateInterface()
{
	mInputChannelInterface.SetTitleAndTooltip( "Serial", "Standard SDI-12" );
	mInputChannelInterface.SetChannel( mInputChannel );

	mBitRateInterface.SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface.SetMax( 1200 );
	mBitRateInterface.SetMin( 1200 );
	mBitRateInterface.SetInteger( mBitRate );

	AddInterface( &mInputChannelInterface );
	AddInterface( &mBitRateInterface );

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

	ClearChannels();
	AddChannel( mInputChannel, "SDI-12", true );

	return true;
}

void SDI12AnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface.SetChannel( mInputChannel );
	mBitRateInterface.SetInteger( mBitRate );
}

void SDI12AnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;

	ClearChannels();
	AddChannel( mInputChannel, "SDI-12", true );

	UpdateInterfacesFromSettings();
}

const char* SDI12AnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;

	return SetReturnString( text_archive.GetString() );
}
