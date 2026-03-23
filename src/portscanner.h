#ifndef portscanner_h_
#define portscanner_h_

#include <wx/string.h>
#include <stdint.h>
#include <serial_cpp/serial.h>
#include "libcalib/protocol.h"

class CPortScanner	// tag = scanner
{
public:
	CPortScanner();
	~CPortScanner();

	void StartScan();				// (re)start parallel probe of all available ports
	void Update();					// call once per timer tick; updates state and reads data
	bool FIsActive() const		{ return m_state == STATE_Active; }
	wxString StrPortActive() const	{ return m_strPortActive; }
	void ForcePort(const wxString & strPort);	// bypass scan; connect this port directly
	void RestartScan();				// close active port (if any) and call StartScan()
	void CloseActive();

	void SendCalibration();			// send mag calibration via protocol manager

private:
	void UpdateState();				// drives state machine
	void ReadFromActive();			// read, parse, dispatch
	enum STATE
	{
		STATE_Scanning,		// probing candidate ports in parallel
		STATE_Active,	// one port confirmed; reading normally

		STATE_Max,
		STATE_Nil = -1,
	};

	struct SProbePort;				// defined in portscanner.cpp (needs adapter types)

	static const int s_cProbeMax = 16;

	STATE m_state;
	wxString m_strPortActive;					// active port name (empty when scanning)
	SProbePort * m_apProbe[s_cProbeMax];	// active probes during scanning
	int m_cProbe;							// count of active probes
	SProbePort * m_pProbeActive;			// winning probe (during STATE_Active)

	void CloseAll();					// delete all probes and zero m_cProbe
};

// Singleton accessor

CPortScanner & Scanner();

#endif // portscanner_h_
