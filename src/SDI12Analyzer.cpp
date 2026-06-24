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

    samples_per_bit = sample_rate_hz / mSettings.mBitRate;
    samples_per_half_bit = U64( 0.5 * double( samples_per_bit ) );

    // A character frame is 10 bit-times (start + 7 data + parity + stop) and always ends
    // with a marking stop bit, so the line can be in continuous spacing for at most ~9
    // bit-times (~7.5 ms @ 1200 baud) during valid traffic. Anything longer is a break.
    // The SDI-12 break is >= 12 ms, so 11 bit-times sits safely above the worst-case
    // character and well below a real break. (The old 6 ms threshold would mis-detect an
    // all-spacing character as a break.)
    minimum_break_width = 11 * samples_per_bit;
    minimum_mark_width = U64( 0.00833 * double( sample_rate_hz ) ); // 8.33 ms
}

void SDI12Analyzer::WorkerThread()
{
    U32 state = LOOKING_FOR_BREAK;

    for( ;; )
    {
        switch( state )
        {
        case LOOKING_FOR_BREAK:
            // Scan edges (without emitting data frames) until the first break appears.
            if( AdvanceToEndOfBreak() )
            {
                state = RECORDER_COMMAND;
            }
            break;

        case RECORDER_COMMAND:
            // ReadNextWord also recognises a break, so a break that begins a new
            // transaction mid-stream is shown as '^' rather than mis-decoded as 0x00.
            if( ReadNextWord() )
            {
                state = RECORDER_COMMAND; // a fresh break restarts the command phase
            }
            else if( AtMark() )
            {
                state = SENSOR_RESPONSE;
            }
            break;

        case SENSOR_RESPONSE:
            if( ReadNextWord() )
            {
                state = RECORDER_COMMAND; // break -> start of a new transaction
            }
            else if( AtMark() )
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

void SDI12Analyzer::EmitBreakFrame( U64 starting_sample, U64 ending_sample )
{
    if( !mSettings.mShowBreak )
    {
        return;
    }

    Frame frame;
    frame.mData1 = '^';
    frame.mType = 0;
    frame.mFlags = FLAG_BREAK;
    frame.mStartingSampleInclusive = starting_sample;
    frame.mEndingSampleInclusive = ending_sample;
    mResults->AddFrame( frame );
    mResults->CommitResults();
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
        EmitBreakFrame( mSerial->GetSampleNumber(), mSerial->GetSampleOfNextEdge() );
        found = true;
    }
    mSerial->AdvanceToNextEdge();
    ReportProgress( mSerial->GetSampleNumber() );
    return found;
}

bool SDI12Analyzer::ReadNextWord()
{
    // Read in the next word
    U8 data = 0x7F;
    U8 parity_count = 0;
    mSerial->AdvanceToNextEdge(); // rising edge -- beginning of the start bit (or a break)

    // A break is a spacing (high) condition longer than any valid character frame. If we
    // decoded it as data, all seven bits of 0x7F would be cleared, yielding a bogus 0x00
    // ('\0'). Detect it here and emit a proper break frame ('^') instead.
    U64 high_run = mSerial->GetSampleOfNextEdge() - mSerial->GetSampleNumber();
    if( high_run > minimum_break_width )
    {
        EmitBreakFrame( mSerial->GetSampleNumber(), mSerial->GetSampleOfNextEdge() );
        mSerial->AdvanceToNextEdge(); // step past the end of the break
        ReportProgress( mSerial->GetSampleNumber() );
        return true;
    }

    U64 starting_sample = mSerial->GetSampleNumber();
    mResults->AddMarker( mSerial->GetSampleNumber() + samples_per_half_bit, AnalyzerResults::Start, mSettings.mInputChannel );

    mSerial->Advance( samples_per_bit + samples_per_half_bit );

    for( U32 i = 0; i < 7; i++ )
    {
        // let's put a dot exactly where we sample this bit:
        mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mInputChannel );

        // Data comes in as least significant to most significant, low is 1
        if( mSerial->GetBitState() == BIT_HIGH )
        {
            data &= ~( ( U32 )1 << i );
            parity_count++;
        }

        mSerial->Advance( samples_per_bit );
    }

    // Check parity
    if( mSerial->GetBitState() == BIT_HIGH )
    {
        parity_count++;
    }
    bool parity_good = parity_count % 2 == 0;

    if( parity_good )
    {
        mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::X, mSettings.mInputChannel );
    }
    else
    {
        mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings.mInputChannel );
    }
    // Advance to the stop bit
    mSerial->Advance( samples_per_bit );
    mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings.mInputChannel );

    // we have a byte to save.
    Frame frame;
    frame.mData1 = data;
    frame.mFlags = 0;
    frame.mStartingSampleInclusive = starting_sample;
    frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

    mResults->AddFrame( frame );
    mResults->CommitResults();
    ReportProgress( frame.mEndingSampleInclusive );
    return false; // a normal data word, not a break
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