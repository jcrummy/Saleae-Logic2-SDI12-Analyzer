#include "SDI12Analyzer.h"
#include "SDI12AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SDI12Analyzer::SDI12Analyzer()
:	Analyzer2(),  
	mSettings(),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( &mSettings );
}

SDI12Analyzer::~SDI12Analyzer()
{
	KillThread();
}

void SDI12Analyzer::SetupResults()
{
	// SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the results each time.
	mResults.reset(new SDI12AnalyzerResults( this, &mSettings ));
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings.mInputChannel );
}

void SDI12Analyzer::WorkerThread()
{
	U32 sample_rate_hz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings.mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();

	// This will get us to the end of the break, and the next advance will bring to the start bit.
	mSerial->AdvanceToNextEdge();
	// Detect the start break, which is supposed to be at least 12ms, but sometimes seems a bit shorter than that.
	// U64 start_of_space = mSerial->GetSampleNumber();

	U32 samples_per_bit = sample_rate_hz / mSettings.mBitRate;
	U32 samples_to_first_center_of_first_data_bit = U32( 1.5 * double( sample_rate_hz ) / double( mSettings.mBitRate ) );

	for( ; ; )
	{
		U8 data = 0x7F;
		
		mSerial->AdvanceToNextEdge(); //falling edge -- beginning of the start bit

		U64 starting_sample = mSerial->GetSampleNumber();

		mSerial->Advance( samples_to_first_center_of_first_data_bit );

		for( U32 i=0; i<7; i++ )
		{
			//let's put a dot exactly where we sample this bit:
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mInputChannel );

			// Data comes in as least significant to most significant, low is 1
			if( mSerial->GetBitState() == BIT_HIGH )
				data &= ~((U32)1 << i);

			mSerial->Advance( samples_per_bit );
		}

		// It is already advanced to the parity bit
		// Advance to the stop bit
		mSerial->Advance( samples_per_bit );

		//we have a byte to save. 
		Frame frame;
		frame.mData1 = data;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = starting_sample;
		frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

		mResults->AddFrame( frame );
		mResults->CommitResults();
		ReportProgress( frame.mEndingSampleInclusive );
	}
}

bool SDI12Analyzer::NeedsRerun()
{
	return false;
}

U32 SDI12Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SDI12Analyzer::GetMinimumSampleRateHz()
{
	return mSettings.mBitRate * 4;
}

const char* SDI12Analyzer::GetAnalyzerName() const
{
	return "SDI-12";
}

const char* GetAnalyzerName()
{
	return "SDI-12";
}

Analyzer* CreateAnalyzer()
{
	return new SDI12Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}