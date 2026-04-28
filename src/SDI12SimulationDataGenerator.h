#ifndef SDI12_SIMULATION_DATA_GENERATOR
#define SDI12_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class SDI12AnalyzerSettings;

class SDI12SimulationDataGenerator
{
public:
	SDI12SimulationDataGenerator();
	~SDI12SimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, SDI12AnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	SDI12AnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateSerialByte();
	std::string mSerialText;
	U32 mStringIndex;

	SimulationChannelDescriptor mSerialSimulationData;

};
#endif //SDI12_SIMULATION_DATA_GENERATOR