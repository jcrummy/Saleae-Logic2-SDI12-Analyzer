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
}

void SDI12Analyzer::WorkerThread()
{
    U32 state = LOOKING_FOR_BREAK;
    U32 sample_rate_hz = GetSampleRate();

    mSerial = GetAnalyzerChannelData( mSettings.mInputChannel );

    U64 minimum_break_width = U64( 0.006 * double( sample_rate_hz ) ); // 6.6 ms
    U64 minimum_mark_width = U64( 0.00833 * double( sample_rate_hz ) ); // 8.33 ms

    U64 samples_per_bit = sample_rate_hz / mSettings.mBitRate;
    U64 samples_per_half_bit = U64( 0.5 * double( samples_per_bit ) );
    U64 samples_to_first_center_of_first_data_bit = U64( 1.5 * double( sample_rate_hz ) / double( mSettings.mBitRate ) );

    // We want to check for a break to start, which means we need a rising edge,
    // so always advance to that to start with.
    if( mSerial->GetBitState() == BIT_LOW )
        mSerial->AdvanceToNextEdge();

    U64 prev_sample = mSerial->GetSampleNumber();

    U64 current_sample = 0;
    U64 length_of_sample = 0;
    U64 starting_sample = 0;
    U8 data = 0;

    Frame frame;

    for( ;; )
    {
        switch( state )
        {
        case LOOKING_FOR_BREAK:
            if( mSerial->GetBitState() == BIT_LOW ) {
                mSerial->AdvanceToNextEdge();
				prev_sample = mSerial->GetSampleNumber();
			}

            // Look for a break, which is a high signal at least 6.6 ms long.
            // The spec is 12 ms, but as a viewer, we have more relaxed tolerances than a recorder does.
            mSerial->AdvanceToNextEdge(); // falling edge -- hopefully the end of the break
            current_sample = mSerial->GetSampleNumber();

            length_of_sample = ( current_sample - prev_sample );
            if( length_of_sample > minimum_break_width )
            {
                if( mSettings.mShowBreak )
                {
                    Frame frame;
                    frame.mData1 = '^';
                    frame.mType = AnalyzerResults::MarkerType::Start;
                    frame.mFlags = 0;
                    frame.mStartingSampleInclusive = prev_sample;
                    frame.mEndingSampleInclusive = current_sample;
                    mResults->AddFrame( frame );
                    mResults->CommitResults();
                }
                ReportProgress( current_sample );
                state = RECORDER_COMMAND;
            }
            else
            {
                // This means we don't actually have a break, so we are probably starting
                // somewhere in the middle of a message. Keep going until we find one.
                mSerial->AdvanceToNextEdge(); // Make sure we start a loop at a rising edge
            }
            prev_sample = mSerial->GetSampleNumber();
            break;

        case RECORDER_COMMAND:

            // Read the next word
            data = 0x7F;
            mSerial->AdvanceToNextEdge(); // rising edge -- beginning of the start bit

            starting_sample = mSerial->GetSampleNumber();

            mResults->AddMarker( starting_sample + U64( 0.5 * double( samples_per_bit ) ), AnalyzerResults::Start,
                                 mSettings.mInputChannel );

            mSerial->Advance( samples_to_first_center_of_first_data_bit );

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
            mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::X, mSettings.mInputChannel );
            // Advance to the stop bit
            mSerial->Advance( samples_per_bit );
            mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings.mInputChannel );

            // we have a byte to save.
            frame.mData1 = data;
            frame.mFlags = 0;
            frame.mStartingSampleInclusive = starting_sample;
            frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

            mResults->AddFrame( frame );
            mResults->CommitResults();
            ReportProgress( frame.mEndingSampleInclusive );

            // Check the distance to the next edge, which, if > 8.33 ms away, indicates
            // the we have reached the end of the recorder command.
            length_of_sample = ( mSerial->GetSampleOfNextEdge() - mSerial->GetSampleNumber() );
            if( length_of_sample > minimum_mark_width )
            {
                state = SENSOR_RESPONSE;
            }
            break;

        case SENSOR_RESPONSE:

            // Read in the next word
            data = 0x7F;
            mSerial->AdvanceToNextEdge(); // rising edge -- beginning of the start bit

            starting_sample = mSerial->GetSampleNumber();

            mSerial->Advance( samples_to_first_center_of_first_data_bit );

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
            prev_sample = mSerial->GetSampleNumber();

            // we have a byte to save.
            frame.mData1 = data;
            frame.mFlags = 0;
            frame.mStartingSampleInclusive = starting_sample;
            frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

            mResults->AddFrame( frame );
            mResults->CommitResults();
            ReportProgress( frame.mEndingSampleInclusive );

            // Check the distance to the next edge, which, if > 8.33 ms away, indicates
            // the we have reached the end of the recorder command.
            length_of_sample = ( mSerial->GetSampleOfNextEdge() - mSerial->GetSampleNumber() );
            if( length_of_sample > minimum_mark_width )
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

void SDI12Analyzer::ReadNextWord()
{
    // // Read in the next word
    // U8 data = 0x7F;
    // mSerial->AdvanceToNextEdge(); // rising edge -- beginning of the start bit

    // U64 starting_sample = mSerial->GetSampleNumber();

    // mSerial->Advance( samples_to_first_center_of_first_data_bit );

    // for( U32 i = 0; i < 7; i++ )
    // {
    //     // let's put a dot exactly where we sample this bit:
    //     mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mInputChannel );

    //     // Data comes in as least significant to most significant, low is 1
    //     if( mSerial->GetBitState() == BIT_HIGH )
    //         data &= ~( ( U32 )1 << i );

    //     mSerial->Advance( samples_per_bit );
    // }

    // // It is already advanced to the parity bit
    // // Advance to the stop bit
    // mSerial->Advance( samples_per_bit );

    // // we have a byte to save.
    // Frame frame;
    // frame.mData1 = data;
    // frame.mFlags = 0;
    // frame.mStartingSampleInclusive = starting_sample;
    // frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

    // mResults->AddFrame( frame );
    // mResults->CommitResults();
    // ReportProgress( frame.mEndingSampleInclusive );
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