#include "portscanner.h"
#include "gui.h"
#include "libcalib/mag_calibrator.h"
#include <cassert>
#include <algorithm>

#define TWEAKABLE static const

TWEAKABLE int s_cTicksScanTimeout = 36;		// ~500ms at 14ms/tick

// --- Protocol adapter: IReader over serial_cpp::Serial ---

class CSerialReader : public libcalib::Protocol::IReader	// tag = srdr
{
public:
	CSerialReader() : m_pSerial(nullptr) {}

	void SetSerial(serial_cpp::Serial * pSerial)	{ m_pSerial = pSerial; }

	size_t CbRead(size_t cBMax, uint8_t * pB) override
	{
		if (m_pSerial == nullptr)
			return 0;

		try
		{
			size_t cBAvail = m_pSerial->available();
			if (cBAvail == 0)
				return 0;

			return m_pSerial->read(pB, std::min(cBAvail, cBMax));
		}
		catch (...)
		{
			return 0;
		}
	}

private:
	serial_cpp::Serial *	m_pSerial;
};

// --- Protocol adapter: IWriter over serial_cpp::Serial ---

class CSerialWriter : public libcalib::Protocol::IWriter	// tag = swtr
{
public:
	CSerialWriter() : m_pSerial(nullptr) {}

	void SetSerial(serial_cpp::Serial * pSerial)	{ m_pSerial = pSerial; }

	void Write(size_t cB, const uint8_t * pB) override
	{
		if (m_pSerial == nullptr)
			return;

		try
		{
			m_pSerial->write(pB, cB);
		}
		catch (...)
		{
		}
	}

private:
	serial_cpp::Serial *	m_pSerial;
};

// --- Protocol adapter: IReceiver for active port ---

void calibration_confirmed();	// defined in gui.cpp

class CPortReceiver : public libcalib::Protocol::IReceiver	// tag = prcvr
{
public:
	CPortReceiver() : m_fCalSent(false), m_calSent() {}

	void OnSample(const libcalib::SSample & samp) override
	{
		libcalib::Mag::CCalibrator & calib = libcalib::Mag::CCalibrator::Ensure();
		calib.AddSample(samp);
	}

	void OnMagCal(const libcalib::Mag::SCal & cal) override
	{
		// compare echoed calibration against what we sent
		if (!m_fCalSent)
			return;

		// NOTE(claude) approximate comparison — the device echoes floats through
		// text formatting, so exact equality is not guaranteed
		if (FCalMatches(cal, m_calSent))
		{
			m_fCalSent = false;
			calibration_confirmed();
		}
	}

	// called by SendCalibration() to record what was sent for confirmation matching
	void NoteSent(const libcalib::Mag::SCal & cal)
	{
		m_calSent = cal;
		m_fCalSent = true;
	}

private:
	static bool FFloatOk(float actual, float expected)
	{
		float err = fabsf(actual - expected);
		float maxerr = 0.0001f + fabsf(expected) * 0.00003f;
		return err <= maxerr;
	}

	static bool FPointMatches(const libcalib::SPoint & a, const libcalib::SPoint & b)
	{
		return FFloatOk(a.x, b.x) && FFloatOk(a.y, b.y) && FFloatOk(a.z, b.z);
	}

	static bool FCalMatches(const libcalib::Mag::SCal & a, const libcalib::Mag::SCal & b)
	{
		return FPointMatches(a.m_vecV, b.m_vecV)
			&& FFloatOk(a.m_sB, b.m_sB)
			&& FPointMatches(a.m_matWInv.vecX, b.m_matWInv.vecX)
			&& FPointMatches(a.m_matWInv.vecY, b.m_matWInv.vecY)
			&& FPointMatches(a.m_matWInv.vecZ, b.m_matWInv.vecZ);
	}

	bool				m_fCalSent;
	libcalib::Mag::SCal	m_calSent;
};

// --- SProbePort: per-port state during scanning and active use ---

struct CPortScanner::SProbePort	// tag = probe
{
	SProbePort()
	: m_protomgr(libcalib::Protocol::VER_MotionCal),
	  m_cTicksOpen(0)
	{
		m_reader.SetSerial(&m_serial);
		m_writer.SetSerial(&m_serial);
		m_protomgr.Init(&m_writer, &m_reader, nullptr);	// no receiver during scanning
	}

	// call after scan wins to attach the receiver for active-port dispatching
	void InitForActive(CPortReceiver * pReceiver)
	{
		m_protomgr.Init(&m_writer, &m_reader, pReceiver);
	}

	wxString							m_strName;		// port device path
	serial_cpp::Serial					m_serial;		// serial instance
	CSerialReader						m_reader;		// IReader adapter
	CSerialWriter						m_writer;		// IWriter adapter
	libcalib::Protocol::CManager		m_protomgr;		// protocol manager
	int									m_cTicksOpen;	// ticks elapsed since port was opened
};

// --- singleton ---

static CPortScanner g_scanner;

CPortScanner & Scanner()
{
	return g_scanner;
}

// --- file-scope receiver instance for the active port ---

static CPortReceiver g_receiver;

// --- CPortScanner ---

CPortScanner::CPortScanner()
	: m_state(STATE_Scanning),
	  m_cProbe(0),
	  m_pProbeActive(nullptr)
{
	for (int i = 0; i < s_cProbeMax; i++)
		m_apProbe[i] = nullptr;
}

CPortScanner::~CPortScanner()
{
	CloseAll();
	CloseActive();
}

void CPortScanner::CloseAll()
{
	for (int i = 0; i < m_cProbe; i++)
	{
		try
		{
			if (m_apProbe[i]->m_serial.isOpen())
				m_apProbe[i]->m_serial.close();
		}
		catch (...) {}

		delete m_apProbe[i];
		m_apProbe[i] = nullptr;
	}

	m_cProbe = 0;
}

void CPortScanner::CloseActive()
{
	if (m_pProbeActive != nullptr)
	{
		try
		{
			if (m_pProbeActive->m_serial.isOpen())
				m_pProbeActive->m_serial.close();
		}
		catch (...) {}

		delete m_pProbeActive;
		m_pProbeActive = nullptr;
	}

	m_strPortActive = wxEmptyString;
	m_state = STATE_Scanning;
}

void CPortScanner::StartScan()
{
	CloseAll();

	m_strPortActive = wxEmptyString;
	m_state = STATE_Scanning;

	wxArrayString aryStrPort = serial_port_list();
	int cPorts = aryStrPort.GetCount();

	for (int i = 0; i < cPorts && m_cProbe < s_cProbeMax; i++)
	{
		SProbePort * pProbe = new SProbePort();
		pProbe->m_strName = aryStrPort[i];
		pProbe->m_cTicksOpen = 0;

		try
		{
			pProbe->m_serial.setPort(aryStrPort[i].ToStdString());
			pProbe->m_serial.setBaudrate(115200);
			serial_cpp::Timeout timeout = serial_cpp::Timeout::simpleTimeout(50);
			pProbe->m_serial.setTimeout(timeout);
			pProbe->m_serial.open();
		}
		catch (...)
		{
			delete pProbe;
			continue;
		}

		m_apProbe[m_cProbe] = pProbe;
		m_cProbe++;
	}
}

void CPortScanner::Update()
{
	UpdateState();
	ReadFromActive();
}

void CPortScanner::UpdateState()
{
	if (m_state == STATE_Scanning)
	{
		bool fAllTimedOut = true;

		for (int iProbe = 0; iProbe < m_cProbe; iProbe++)
		{
			SProbePort * pProbe = m_apProbe[iProbe];

			try
			{
				// let the protocol manager read and detect
				pProbe->m_protomgr.Update();

				if (pProbe->m_protomgr.FHasRemote())
				{
					// This probe wins — move it to active, delete all others

					m_strPortActive = pProbe->m_strName;
					m_pProbeActive = pProbe;
					m_apProbe[iProbe] = nullptr;

					// attach receiver for active-port dispatching
					m_pProbeActive->InitForActive(&g_receiver);

					// Close and delete all other probes

					for (int j = 0; j < m_cProbe; j++)
					{
						if (m_apProbe[j] == nullptr)
							continue;

						try
						{
							if (m_apProbe[j]->m_serial.isOpen())
								m_apProbe[j]->m_serial.close();
						}
						catch (...) {}

						delete m_apProbe[j];
						m_apProbe[j] = nullptr;
					}

					m_cProbe = 0;
					m_state = STATE_Active;
					return;
				}
			}
			catch (...)
			{
				// Probe failed — close and remove it

				try
				{
					if (pProbe->m_serial.isOpen())
						pProbe->m_serial.close();
				}
				catch (...) {}

				delete pProbe;

				// Shift remaining probes down

				for (int j = iProbe; j < m_cProbe - 1; j++)
					m_apProbe[j] = m_apProbe[j + 1];

				m_apProbe[m_cProbe - 1] = nullptr;
				m_cProbe--;
				iProbe--;	// re-check this index
				continue;
			}

			pProbe->m_cTicksOpen++;
			if (pProbe->m_cTicksOpen < s_cTicksScanTimeout)
				fAllTimedOut = false;
		}

		// If all probes timed out (or no probes), re-enumerate and try again

		if (fAllTimedOut)
		{
			CloseAll();
			StartScan();
		}
	}
	else if (m_state == STATE_Active)
	{
		if (m_pProbeActive == nullptr || !m_pProbeActive->m_serial.isOpen())
		{
			CloseActive();
			StartScan();
		}
	}
}

void CPortScanner::ForcePort(const wxString & strPort)
{
	CloseAll();
	CloseActive();

	SProbePort * pProbe = new SProbePort();
	pProbe->m_strName = strPort;
	pProbe->m_cTicksOpen = 0;

	try
	{
		pProbe->m_serial.setPort(strPort.ToStdString());
		pProbe->m_serial.setBaudrate(115200);
		serial_cpp::Timeout timeout = serial_cpp::Timeout::simpleTimeout(50);
		pProbe->m_serial.setTimeout(timeout);
		pProbe->m_serial.open();
	}
	catch (...)
	{
		delete pProbe;
		return;
	}

	// Directly connect — skip probing, attach receiver

	pProbe->InitForActive(&g_receiver);

	m_pProbeActive = pProbe;
	m_strPortActive = strPort;
	m_state = STATE_Active;
}

void CPortScanner::RestartScan()
{
	CloseAll();
	CloseActive();
	StartScan();
}

void CPortScanner::ReadFromActive()
{
	if (!FIsActive() || m_pProbeActive == nullptr)
		return;

	try
	{
		m_pProbeActive->m_protomgr.Update();
	}
	catch (...)
	{
		CloseActive();
	}
}

void CPortScanner::SendCalibration()
{
	if (!FIsActive() || m_pProbeActive == nullptr)
		return;

	const auto & calib = libcalib::Mag::CCalibrator::Ensure();
	const auto & cal = calib.m_fitter.m_cal;

	g_receiver.NoteSent(cal);
	m_pProbeActive->m_protomgr.SendMagCal(cal);
}
