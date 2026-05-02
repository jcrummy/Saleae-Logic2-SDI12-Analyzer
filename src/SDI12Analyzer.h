#ifndef SDI12_ANALYZER_H
#define SDI12_ANALYZER_H

#include <Analyzer.h>
#include "SDI12AnalyzerSettings.h"
#include "SDI12AnalyzerResults.h"
#include "SDI12SimulationDataGenerator.h"
#include <memory>

enum AnalyzerState {
	LOOKING_FOR_BREAK,
	RECORDER_COMMAND,
	SENSOR_RESPONSE
};

class ANALYZER_EXPORT SDI12Analyzer : public Analyzer2
{
public:
	SDI12Analyzer();
	virtual ~SDI12Analyzer();

	virtual void SetupResults();
	virtual void WorkerThread();
	virtual U8 ReadNextWord();
	virtual bool AtMark();
	virtual bool AdvanceToEndOfBreak();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	SDI12AnalyzerSettings mSettings;
	std::unique_ptr<SDI12AnalyzerResults> mResults;
	AnalyzerChannelData* mSerial;

	SDI12SimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	U64 samples_per_bit;
	U64 samples_per_half_bit;
	U64 minimum_break_width;
	U64 minimum_mark_width;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SDI12_ANALYZER_H
