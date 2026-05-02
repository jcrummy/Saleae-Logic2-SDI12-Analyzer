#include "SDI12Analyzer.h"
#include "SDI12AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SDI12Analyzer::SDI12Analyzer() : Analyzer2(), mSettings(), mSimulationInitilized( false )
{
    SetAnalyzerSettings( &mSettings );
}

SDI12Analyzer::~SDI12Analyzer()
{
    KillThread();
}

void SDI12Analyzer::SetupResults()
{
    // SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the
    // results each time.
    mResults.reset( new SDI12AnalyzerResults( this, &mSettings ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings.mInputChannel );

    mSerial = GetAnalyzerChannelData( mSettings.mInputChannel );

    U32 sample_rate_hz = GetSampleRate();
    minimum_break_width = U64( 0.006 * double( sample_rate_hz ) );  // 6.6 ms
    minimum_mark_width = U64( 0.00833 * double( sample_rate_hz ) ); // 8.33 ms

    samples_per_bit = sample_rate_hz / mSettings.mBitRate;
    samples_per_half_bit = U64( 0.5 * double( samples_per_bit ) );
}

void SDI12Analyzer::WorkerThread()
{
    U32 state = LOOKING_FOR_BREAK;

    for( ;; )
    {
        switch( state )
        {
        case LOOKING_FOR_BREAK:
            if( AdvanceToEndOfBreak() )
            {
                state = RECORDER_COMMAND;
            }
            break;

        case RECORDER_COMMAND:

            // Read the next word
            ReadNextWord();

            if( AtMark() )
            {
                state = SENSOR_RESPONSE;
            }
            break;

        case SENSOR_RESPONSE:

            // Read in the next word
            ReadNextWord();

            if( AtMark() )
            {
                state = LOOKING_FOR_BREAK;
            }
            break;

        default:
            state = LOOKING_FOR_BREAK;
            break;
        }
    }
}

bool SDI12Analyzer::AdvanceToEndOfBreak()
{
    // Breaks always start with a rising edge, so advance to the next one
    if( mSerial->GetBitState() == BIT_LOW )
    {
        mSerial->AdvanceToNextEdge();
    }

    bool found = false;

    U64 length_of_sample = mSerial->GetSampleOfNextEdge() - mSerial->GetSampleNumber();
    if( length_of_sample > minimum_break_width )
    {
        if( mSettings.mShowBreak )
        {
            Frame frame;
            frame.mData1 = '^';
            frame.mType = AnalyzerResults::MarkerType::Start;
            frame.mFlags = 0;
            frame.mStartingSampleInclusive = mSerial->GetSampleNumber();
            frame.mEndingSampleInclusive = mSerial->GetSampleOfNextEdge();
            mResults->AddFrame( frame );
            mResults->CommitResults();
        }
        found = true;
    }
    mSerial->AdvanceToNextEdge();
    ReportProgress( mSerial->GetSampleNumber() );
    return found;
}

U8 SDI12Analyzer::ReadNextWord()
{
    // Read in the next word
    U8 data = 0x7F;
    mSerial->AdvanceToNextEdge(); // rising edge -- beginning of the start bit

    U64 starting_sample = mSerial->GetSampleNumber();

    mSerial->Advance( samples_per_bit + samples_per_half_bit );

    for( U32 i = 0; i < 7; i++ )
    {
        // let's put a dot exactly where we sample this bit:
        mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mInputChannel );

        // Data comes in as least significant to most significant, low is 1
        if( mSerial->GetBitState() == BIT_HIGH )
            data &= ~( ( U32 )1 << i );

        mSerial->Advance( samples_per_bit );
    }

    // It is already advanced to the parity bit
    // Advance to the stop bit
    mSerial->Advance( samples_per_bit );

    // we have a byte to save.
    Frame frame;
    frame.mData1 = data;
    frame.mFlags = 0;
    frame.mStartingSampleInclusive = starting_sample;
    frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

    mResults->AddFrame( frame );
    mResults->CommitResults();
    ReportProgress( frame.mEndingSampleInclusive );
	return data;
}

bool SDI12Analyzer::AtMark()
{
    U64 length_of_sample = ( mSerial->GetSampleOfNextEdge() - mSerial->GetSampleNumber() );
    if( length_of_sample > minimum_mark_width )
    {
        return true;
    }
    return false;
}

bool SDI12Analyzer::NeedsRerun()
{
    return false;
}

U32 SDI12Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                           SimulationChannelDescriptor** simulation_channels )
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