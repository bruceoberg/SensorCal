#include "lineparser.h"
#include <cstring>
#include <cstdlib>

// Raw: line int16 -> float conversion.
// These must match the encoding in imucal.ino (Adafruit imucal firmware).
// The imucal encoding includes unit conversions as a side effect:
//   accel: m/s² × (8192/9.8) → int16, so decoding gives g (not m/s²)
//   gyro: deg/s × 16 → int16, so decoding gives deg/s
//   mag: µT × 10 → int16, so decoding gives µT

static constexpr float SAccelFromS16(int16_t s)
{
	return float(s) * (1.0f / 8192.0f);		// returns g
}

static constexpr float SGyroFromS16(int16_t s)
{
	return float(s) * (1.0f / 16.0f);		// returns deg/s
}

static constexpr float SMagFromS16(int16_t s)
{
	return float(s) * (1.0f / 10.0f);		// returns uT
}

CLineParser::CLineParser()
{
	Reset();
}

void CLineParser::Reset()
{
	m_cChLine = 0;
	m_linetype = LINETYPE_None;
	memset(&m_samp, 0, sizeof(m_samp));
	memset(m_aGCal, 0, sizeof(m_aGCal));
	m_cGCal = 0;
}

CLineParser::LINETYPE CLineParser::LinetypeFeedBytes(const uint8_t * pB, int cB)
{
	LINETYPE linetypeResult = LINETYPE_None;

	for (int iB = 0; iB < cB; ++iB)
	{
		char ch = static_cast<char>(pB[iB]);

		// CR marks end of line — parse and reset

		if (ch == '\r')
		{
			m_aChLine[m_cChLine] = '\0';
			linetypeResult = LinetypeParseLine();
			m_cChLine = 0;
		}
		else if (ch == '\n')
		{
			// ignore LF
		}
		else if (m_cChLine < s_cChLineMax - 1)
		{
			m_aChLine[m_cChLine++] = ch;
		}
		else
		{
			// line too long — discard and reset

			m_cChLine = 0;
		}
	}

	return linetypeResult;
}

CLineParser::LINETYPE CLineParser::LinetypeParseLine()
{
	m_linetype = LINETYPE_None;

	// Match line prefix

	if (strncmp(m_aChLine, "Uni:", 4) == 0)
	{
		if (FParseUni(m_aChLine + 4))
			m_linetype = LINETYPE_Uni;
	}
	else if (strncmp(m_aChLine, "Raw:", 4) == 0)
	{
		if (FParseRaw(m_aChLine + 4))
			m_linetype = LINETYPE_Raw;
	}
	else if (strncmp(m_aChLine, "Cal1:", 5) == 0)
	{
		if (FParseCal(m_aChLine + 5, 10))
		{
			m_linetype = LINETYPE_Cal1;
			m_cGCal = 10;
		}
	}
	else if (strncmp(m_aChLine, "Cal2:", 5) == 0)
	{
		if (FParseCal(m_aChLine + 5, 9))
		{
			m_linetype = LINETYPE_Cal2;
			m_cGCal = 9;
		}
	}

	return m_linetype;
}

bool CLineParser::FParseUni(const char * pCh)
{
	// Parse 9 comma-separated floats: ax,ay,az,gx,gy,gz,mx,my,mz

	float aG[9];
	char * pChEnd = nullptr;

	for (int iG = 0; iG < 9; ++iG)
	{
		aG[iG] = strtof(pCh, &pChEnd);

		if (pChEnd == pCh)
			return false;	// no digits parsed

		pCh = pChEnd;

		if (iG < 8)
		{
			if (*pCh != ',')
				return false;
			++pCh;
		}
	}

	// Convert from Uni: units (m/s^2, rad/s, uT) to SSample units (g, deg/s, uT)

	m_samp.m_pntAccel = libcalib::SPoint(
		libcalib::GFromMPerSecSq(aG[0]),
		libcalib::GFromMPerSecSq(aG[1]),
		libcalib::GFromMPerSecSq(aG[2]));
	m_samp.m_pntGyro = libcalib::SPoint(
		libcalib::DegFromRad(aG[3]),
		libcalib::DegFromRad(aG[4]),
		libcalib::DegFromRad(aG[5]));
	m_samp.m_pntMag = libcalib::SPoint(aG[6], aG[7], aG[8]);

	return true;
}

bool CLineParser::FParseRaw(const char * pCh)
{
	// Parse 9 comma-separated int16s, convert to floats

	int16_t aN[9];
	char * pChEnd = nullptr;

	for (int iN = 0; iN < 9; ++iN)
	{
		long n = strtol(pCh, &pChEnd, 10);

		if (pChEnd == pCh)
			return false;	// no digits parsed

		// Range check for int16

		if (n < -32768 || n > 32767)
			return false;

		aN[iN] = static_cast<int16_t>(n);
		pCh = pChEnd;

		if (iN < 8)
		{
			if (*pCh != ',')
				return false;
			++pCh;
		}
	}

	// Convert to physical units

	m_samp.m_pntAccel = libcalib::SPoint(
		SAccelFromS16(aN[0]),
		SAccelFromS16(aN[1]),
		SAccelFromS16(aN[2]));
	m_samp.m_pntGyro = libcalib::SPoint(
		SGyroFromS16(aN[3]),
		SGyroFromS16(aN[4]),
		SGyroFromS16(aN[5]));
	m_samp.m_pntMag = libcalib::SPoint(
		SMagFromS16(aN[6]),
		SMagFromS16(aN[7]),
		SMagFromS16(aN[8]));

	return true;
}

bool CLineParser::FParseCal(const char * pCh, int cGExpected)
{
	// Parse cGExpected comma-separated floats

	char * pChEnd = nullptr;

	for (int iG = 0; iG < cGExpected; ++iG)
	{
		m_aGCal[iG] = strtof(pCh, &pChEnd);

		if (pChEnd == pCh)
			return false;	// no digits parsed

		pCh = pChEnd;

		if (iG < cGExpected - 1)
		{
			if (*pCh != ',')
				return false;
			++pCh;
		}
	}

	return true;
}
