#include "portscanner.h"
#include "gui.h"
#include "imuread.h"
#include <cassert>
#include <algorithm>

#define TWEAKABLE static const

TWEAKABLE int s_cTicksScanTimeout = 36;		// ~500ms at 14ms/tick

static CPortScanner g_scanner;

CPortScanner & Scanner()
{
	return g_scanner;
}

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
				size_t cBAvail = pProbe->m_serial.available();
				if (cBAvail > 0)
				{
					uint8_t aB[256];
					size_t cB = pProbe->m_serial.read(aB, std::min(cBAvail, sizeof(aB)));

					if (cB > 0)
					{
						libcalib::CLineParser::LINETYPE lt = pProbe->m_linep.LinetypeFeedBytes(
							aB, static_cast<int>(cB));

						if (lt == libcalib::CLineParser::LINETYPE_Raw || lt == libcalib::CLineParser::LINETYPE_Uni)
						{
							// This probe wins — move it to active, delete all others

							m_strPortActive = pProbe->m_strName;
							m_pProbeActive = pProbe;
							m_apProbe[iProbe] = nullptr;

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

	// Directly connect — skip probing

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

	uint8_t aB[256];
	size_t cB = 0;

	try
	{
		size_t cBAvail = m_pProbeActive->m_serial.available();
		if (cBAvail == 0)
			return;

		cB = m_pProbeActive->m_serial.read(aB, std::min(cBAvail, sizeof(aB)));
	}
	catch (...)
	{
		CloseActive();
		return;
	}

	if (cB == 0)
		return;

	libcalib::CLineParser::LINETYPE lt = m_pProbeActive->m_linep.LinetypeFeedBytes(
		aB, static_cast<int>(cB));

	if (lt == libcalib::CLineParser::LINETYPE_Uni) // skipping raw for now || lt == libcalib::CLineParser::LINETYPE_Raw)
	{
		libcalib::Mag::CCalibrator & calib = libcalib::Mag::CCalibrator::Ensure();
		calib.AddSample(m_pProbeActive->m_linep.Samp());
	}
	else if (lt == libcalib::CLineParser::LINETYPE_Cal1)
	{
		cal1_data(m_pProbeActive->m_linep.PGCal());
	}
	else if (lt == libcalib::CLineParser::LINETYPE_Cal2)
	{
		cal2_data(m_pProbeActive->m_linep.PGCal());
	}
}

size_t CPortScanner::WriteToActive(int cB, const void * pV)
{
	if (!FIsActive() || m_pProbeActive == nullptr)
		return 0;

	try
	{
		return m_pProbeActive->m_serial.write(
			static_cast<const uint8_t *>(pV),
			static_cast<size_t>(cB));
	}
	catch (...)
	{
		return 0;
	}
}
