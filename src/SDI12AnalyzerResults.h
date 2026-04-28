#ifndef SDI12_ANALYZER_RESULTS
#define SDI12_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class SDI12Analyzer;
class SDI12AnalyzerSettings;

class SDI12AnalyzerResults : public AnalyzerResults
{
public:
	SDI12AnalyzerResults( SDI12Analyzer* analyzer, SDI12AnalyzerSettings* settings );
	virtual ~SDI12AnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	SDI12AnalyzerSettings* mSettings;
	SDI12Analyzer* mAnalyzer;
};

#endif //SDI12_ANALYZER_RESULTS
