#ifndef lineparser_h_
#define lineparser_h_

#include <stdint.h>
#include "libcalib.h"

// CLineParser — line-buffered parser for IMU serial data
//
// Accumulates bytes until CR, then parses complete lines.
// Handles Uni:, Raw:, Cal1:, and Cal2: line formats.
// Per-instance state (no statics) — fully reentrant.

class CLineParser	// tag = linep
{
public:
	enum LINETYPE
	{
		LINETYPE_None,		// no complete line yet
		LINETYPE_Uni,		// Uni: 9 floats (full precision)
		LINETYPE_Raw,		// Raw: 9 int16s (converted to floats)
		LINETYPE_Cal1,		// Cal1: 10 floats (calibration echo)
		LINETYPE_Cal2,		// Cal2: 9 floats (calibration echo)

		LINETYPE_Max,
		LINETYPE_Nil = -1
	};

	CLineParser();
	void Reset();

	// Feed bytes. Returns LINETYPE_None while buffering,
	// or a valid LINETYPE when a complete line is parsed.

	LINETYPE LinetypeFeedBytes(const uint8_t * pB, int cB);

	// Accessors (valid after corresponding line type)

	const libcalib::SSample & Samp() const		{ return m_samp; }
	const float * PGCal() const			{ return m_aGCal; }
	int CGCal() const					{ return m_cGCal; }

private:
	static const int s_cChLineMax = 256;

	char m_aChLine[s_cChLineMax];	// line buffer
	int m_cChLine;					// current line length

	LINETYPE m_linetype;				// type of last parsed line
	libcalib::SSample m_samp;			// parsed sample data
	float m_aGCal[10];				// parsed calibration data
	int m_cGCal;					// count of calibration floats

	LINETYPE LinetypeParseLine();
	bool FParseUni(const char * pCh);
	bool FParseRaw(const char * pCh);
	bool FParseCal(const char * pCh, int cGExpected);
};

#endif // lineparser_h_
