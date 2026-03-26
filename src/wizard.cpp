#include "wizard.h"
#include "portscanner.h"
#include "libcalib/coordinator.h"

// --- Event IDs ---

#define ID_WIZARD_TIMER			11000
#define ID_BTN_ACCEPT_PORT		11001
#define ID_BTN_ACCEPT_GYRO		11002
#define ID_BTN_ACCEPT_ACCEL		11003
#define ID_BTN_ACCEPT_MAG		11004
#define ID_BTN_SEND_CAL			11005

// --- File-static coordinator ---

static libcalib::CCoordinator g_coordinator;

// --- Sample callback — feeds coordinator from the serial path ---

static void OnWizardSample(const libcalib::SSample & samp)
{
	g_coordinator.OnSample(samp);
}

// ============================================================================
// CStepCard
// ============================================================================

CStepCard::CStepCard(
	wxWindow * pParent,
	int nStep,
	const wxString & strName)
: wxPanel(pParent, wxID_ANY),
  m_fExpanded(false),
  m_statusk(STATUSK_ToDo)
{
	auto * pSizerOuter = new wxBoxSizer(wxVERTICAL);

	// Header row

	m_pPanelHeader = new wxPanel(this, wxID_ANY);
	m_pPanelHeader->SetBackgroundColour(wxColour(240, 240, 240));

	auto * pSizerHeader = new wxBoxSizer(wxHORIZONTAL);

	m_pTextNumber = new wxStaticText(m_pPanelHeader, wxID_ANY,
		wxString::Format("%d.", nStep));
	m_pTextName = new wxStaticText(m_pPanelHeader, wxID_ANY, strName);

	// Bold, slightly larger font for header

	wxFont fontHeader = m_pTextName->GetFont();
	fontHeader.SetPointSize(fontHeader.GetPointSize() + 2);
	fontHeader.SetWeight(wxFONTWEIGHT_BOLD);
	m_pTextNumber->SetFont(fontHeader);
	m_pTextName->SetFont(fontHeader);

	m_pTextStatus = new wxStaticText(m_pPanelHeader, wxID_ANY, "To Do");
	m_pTextStatus->SetForegroundColour(wxColour(128, 128, 128));

	pSizerHeader->Add(m_pTextNumber, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
	pSizerHeader->Add(m_pTextName, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
	pSizerHeader->AddStretchSpacer();
	pSizerHeader->Add(m_pTextStatus, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	m_pPanelHeader->SetSizer(pSizerHeader);

	// Bind click on the entire header area

	m_pPanelHeader->Bind(wxEVT_LEFT_UP, &CStepCard::OnHeaderClick, this);
	m_pTextNumber->Bind(wxEVT_LEFT_UP, &CStepCard::OnHeaderClick, this);
	m_pTextName->Bind(wxEVT_LEFT_UP, &CStepCard::OnHeaderClick, this);
	m_pTextStatus->Bind(wxEVT_LEFT_UP, &CStepCard::OnHeaderClick, this);

	// Body panel — hidden by default

	m_pPanelBody = new wxPanel(this, wxID_ANY);

	pSizerOuter->Add(m_pPanelHeader, 0, wxEXPAND | wxBOTTOM, 1);
	pSizerOuter->Add(m_pPanelBody, 0, wxEXPAND);
	SetSizer(pSizerOuter);

	m_pPanelBody->Hide();
}

void CStepCard::Expand()
{
	if (m_fExpanded)
		return;

	m_fExpanded = true;
	m_pPanelBody->Show();
	GetParent()->Layout();
	GetParent()->FitInside();
}

void CStepCard::Collapse()
{
	if (!m_fExpanded)
		return;

	m_fExpanded = false;
	m_pPanelBody->Hide();
	GetParent()->Layout();
	GetParent()->FitInside();
}

void CStepCard::SetStatus(STATUSK statusk)
{
	m_statusk = statusk;

	wxFont font = m_pTextStatus->GetFont();

	switch (statusk)
	{
	case STATUSK_ToDo:
		m_pTextStatus->SetLabel("To Do");
		m_pTextStatus->SetForegroundColour(wxColour(128, 128, 128));
		font.SetStyle(wxFONTSTYLE_NORMAL);
		break;

	case STATUSK_Active:
		m_pTextStatus->SetLabel("Active");
		m_pTextStatus->SetForegroundColour(wxColour(0, 80, 200));
		font.SetStyle(wxFONTSTYLE_NORMAL);
		break;

	case STATUSK_Done:
		m_pTextStatus->SetLabel("Done");
		m_pTextStatus->SetForegroundColour(wxColour(0, 150, 0));
		font.SetStyle(wxFONTSTYLE_NORMAL);
		break;

	case STATUSK_Skipped:
		m_pTextStatus->SetLabel("Skipped");
		m_pTextStatus->SetForegroundColour(wxColour(128, 128, 128));
		font.SetStyle(wxFONTSTYLE_ITALIC);
		break;

	default:
		break;
	}

	m_pTextStatus->SetFont(font);
}

void CStepCard::OnHeaderClick(wxMouseEvent & event)
{
	// Propagate to wizard frame via a command event

	wxCommandEvent cmd(wxEVT_BUTTON, GetId());
	cmd.SetEventObject(this);
	ProcessWindowEvent(cmd);
}

// ============================================================================
// CWizardFrame
// ============================================================================

BEGIN_EVENT_TABLE(CWizardFrame, wxFrame)
	EVT_TIMER(ID_WIZARD_TIMER, CWizardFrame::OnTimer)
	EVT_BUTTON(ID_BTN_ACCEPT_PORT, CWizardFrame::OnAcceptPort)
	EVT_BUTTON(ID_BTN_ACCEPT_GYRO, CWizardFrame::OnAcceptGyro)
	EVT_BUTTON(ID_BTN_ACCEPT_ACCEL, CWizardFrame::OnAcceptAccel)
	EVT_BUTTON(ID_BTN_ACCEPT_MAG, CWizardFrame::OnAcceptMag)
	EVT_BUTTON(ID_BTN_SEND_CAL, CWizardFrame::OnSendCalibration)
END_EVENT_TABLE()

CWizardFrame::CWizardFrame(
	wxWindow * pParent,
	wxWindowID id,
	const wxString & strTitle,
	const wxPoint & pos,
	const wxSize & size)
: wxFrame(pParent, id, strTitle, pos, size, wxDEFAULT_FRAME_STYLE),
  m_iStepActive(-1),
  m_timer(nullptr),
  m_pTextPortStatus(nullptr),
  m_pBtnAcceptPort(nullptr),
  m_pTextGyroInstr(nullptr),
  m_pGaugeGyro(nullptr),
  m_pBtnAcceptGyro(nullptr),
  m_pTextAccelInstr(nullptr),
  m_pGaugeAccelFace(nullptr),
  m_pGaugeAccelTotal(nullptr),
  m_pBtnAcceptAccel(nullptr),
  m_pTextMagInstr(nullptr),
  m_pTextMagGaps(nullptr),
  m_pTextMagVariance(nullptr),
  m_pTextMagWobble(nullptr),
  m_pTextMagFit(nullptr),
  m_pBtnAcceptMag(nullptr),
  m_pTextSendSummary(nullptr),
  m_pBtnSend(nullptr),
  m_pTextSendStatus(nullptr)
{
	SetMinSize(wxSize(400, -1));

	auto * pSizerMain = new wxBoxSizer(wxVERTICAL);

	// Create the five step cards

	static const char * s_apchzName[] =
	{
		"Choose Port",
		"Gyro",
		"Accel",
		"Mag",
		"Send + Confirm",
	};

	for (int i = 0; i < s_cStep; ++i)
	{
		m_apStep[i] = new CStepCard(this, i + 1, s_apchzName[i]);
		pSizerMain->Add(m_apStep[i], 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 6);

		// Bind header click to accordion logic

		m_apStep[i]->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent &) { OnStepHeaderClick(i); });
	}

	pSizerMain->AddStretchSpacer();
	SetSizer(pSizerMain);

	// Build each step's body content

	BuildPortBody(m_apStep[0]->PanelBody());
	BuildGyroBody(m_apStep[1]->PanelBody());
	BuildAccelBody(m_apStep[2]->PanelBody());
	BuildMagBody(m_apStep[3]->PanelBody());
	BuildSendBody(m_apStep[4]->PanelBody());

	// Register sample callback

	SetWizardSampleCallback(OnWizardSample);

	// Start with step 1 expanded

	ExpandStep(0);

	// Timer — same 14ms interval as MyFrame

	m_timer = new wxTimer(this, ID_WIZARD_TIMER);
	m_timer->Start(14, wxTIMER_CONTINUOUS);
}

CWizardFrame::~CWizardFrame()
{
	SetWizardSampleCallback(nullptr);

	if (m_timer)
	{
		m_timer->Stop();
		delete m_timer;
	}
}

// --- Accordion ---

void CWizardFrame::ExpandStep(int iStep)
{
	if (iStep == m_iStepActive)
		return;

	if (iStep < 0 || iStep >= s_cStep)
		return;

	// Collapse current

	if (m_iStepActive >= 0 && m_iStepActive < s_cStep)
		m_apStep[m_iStepActive]->Collapse();

	// Expand new

	m_iStepActive = iStep;
	m_apStep[iStep]->Expand();
	m_apStep[iStep]->SetStatus(STATUSK_Active);

	Layout();
}

// --- Step body builders ---

void CWizardFrame::BuildPortBody(wxPanel * pBody)
{
	auto * pSizer = new wxBoxSizer(wxVERTICAL);

	m_pTextPortStatus = new wxStaticText(pBody, wxID_ANY, "Searching...");
	pSizer->Add(m_pTextPortStatus, 0, wxEXPAND | wxALL, 10);

	m_pBtnAcceptPort = new wxButton(pBody, ID_BTN_ACCEPT_PORT, "Accept");
	m_pBtnAcceptPort->Hide();
	pSizer->Add(m_pBtnAcceptPort, 0, wxLEFT | wxBOTTOM, 10);

	pBody->SetSizer(pSizer);
}

void CWizardFrame::BuildGyroBody(wxPanel * pBody)
{
	auto * pSizer = new wxBoxSizer(wxVERTICAL);

	m_pTextGyroInstr = new wxStaticText(pBody, wxID_ANY, "");
	pSizer->Add(m_pTextGyroInstr, 0, wxEXPAND | wxALL, 10);

	m_pGaugeGyro = new wxGauge(pBody, wxID_ANY, 100);
	pSizer->Add(m_pGaugeGyro, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	m_pBtnAcceptGyro = new wxButton(pBody, ID_BTN_ACCEPT_GYRO, "Accept");
	pSizer->Add(m_pBtnAcceptGyro, 0, wxLEFT | wxTOP | wxBOTTOM, 10);

	pBody->SetSizer(pSizer);
}

void CWizardFrame::BuildAccelBody(wxPanel * pBody)
{
	auto * pSizer = new wxBoxSizer(wxVERTICAL);

	m_pTextAccelInstr = new wxStaticText(pBody, wxID_ANY, "");
	pSizer->Add(m_pTextAccelInstr, 0, wxEXPAND | wxALL, 10);

	auto * pTextFace = new wxStaticText(pBody, wxID_ANY, "Face progress:");
	pSizer->Add(pTextFace, 0, wxLEFT | wxTOP, 10);

	m_pGaugeAccelFace = new wxGauge(pBody, wxID_ANY, 100);
	pSizer->Add(m_pGaugeAccelFace, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	auto * pTextTotal = new wxStaticText(pBody, wxID_ANY, "Overall progress:");
	pSizer->Add(pTextTotal, 0, wxLEFT | wxTOP, 10);

	m_pGaugeAccelTotal = new wxGauge(pBody, wxID_ANY, 100);
	pSizer->Add(m_pGaugeAccelTotal, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

	m_pBtnAcceptAccel = new wxButton(pBody, ID_BTN_ACCEPT_ACCEL, "Accept");
	pSizer->Add(m_pBtnAcceptAccel, 0, wxLEFT | wxTOP | wxBOTTOM, 10);

	pBody->SetSizer(pSizer);
}

void CWizardFrame::BuildMagBody(wxPanel * pBody)
{
	auto * pSizer = new wxBoxSizer(wxVERTICAL);

	m_pTextMagInstr = new wxStaticText(pBody, wxID_ANY,
		"Rotate device in all directions");
	pSizer->Add(m_pTextMagInstr, 0, wxEXPAND | wxALL, 10);

	// Quality metrics grid

	auto * pSizerMetrics = new wxFlexGridSizer(4, 2, 4, 12);

	pSizerMetrics->Add(new wxStaticText(pBody, wxID_ANY, "Gaps:"));
	m_pTextMagGaps = new wxStaticText(pBody, wxID_ANY, "--");
	pSizerMetrics->Add(m_pTextMagGaps);

	pSizerMetrics->Add(new wxStaticText(pBody, wxID_ANY, "Variance:"));
	m_pTextMagVariance = new wxStaticText(pBody, wxID_ANY, "--");
	pSizerMetrics->Add(m_pTextMagVariance);

	pSizerMetrics->Add(new wxStaticText(pBody, wxID_ANY, "Wobble:"));
	m_pTextMagWobble = new wxStaticText(pBody, wxID_ANY, "--");
	pSizerMetrics->Add(m_pTextMagWobble);

	pSizerMetrics->Add(new wxStaticText(pBody, wxID_ANY, "Fit:"));
	m_pTextMagFit = new wxStaticText(pBody, wxID_ANY, "--");
	pSizerMetrics->Add(m_pTextMagFit);

	pSizer->Add(pSizerMetrics, 0, wxLEFT | wxRIGHT, 10);

	// BB(claude) GL sphere canvas placeholder — add OpenGL visualization here

	m_pBtnAcceptMag = new wxButton(pBody, ID_BTN_ACCEPT_MAG, "Accept");
	pSizer->Add(m_pBtnAcceptMag, 0, wxLEFT | wxTOP | wxBOTTOM, 10);

	pBody->SetSizer(pSizer);
}

void CWizardFrame::BuildSendBody(wxPanel * pBody)
{
	auto * pSizer = new wxBoxSizer(wxVERTICAL);

	m_pTextSendSummary = new wxStaticText(pBody, wxID_ANY, "");
	pSizer->Add(m_pTextSendSummary, 0, wxEXPAND | wxALL, 10);

	m_pBtnSend = new wxButton(pBody, ID_BTN_SEND_CAL, "Send Calibration");
	pSizer->Add(m_pBtnSend, 0, wxLEFT | wxBOTTOM, 10);

	m_pTextSendStatus = new wxStaticText(pBody, wxID_ANY, "");
	pSizer->Add(m_pTextSendStatus, 0, wxLEFT | wxBOTTOM, 10);

	pBody->SetSizer(pSizer);
}

// --- Button handlers ---

void CWizardFrame::OnAcceptPort(wxCommandEvent & event)
{
	if (!Scanner().FIsActive())
		return;

	m_apStep[0]->SetStatus(STATUSK_Done);

	// Start the coordinator at gyro phase

	g_coordinator.Start();
	ExpandStep(1);
}

void CWizardFrame::OnAcceptGyro(wxCommandEvent & event)
{
	auto phase = g_coordinator.Phase();

	if (phase == libcalib::CCoordinator::PHASE_Gyro)
	{
		// Early accept — skip to accel, preserving partial gyro bias

		g_coordinator.StartAtPhase(libcalib::CCoordinator::PHASE_Accel);
		m_apStep[1]->SetStatus(STATUSK_Skipped);
	}
	else
	{
		m_apStep[1]->SetStatus(STATUSK_Done);
	}

	ExpandStep(2);
}

void CWizardFrame::OnAcceptAccel(wxCommandEvent & event)
{
	auto phase = g_coordinator.Phase();

	if (phase == libcalib::CCoordinator::PHASE_Accel)
	{
		// Early accept — skip to mag

		g_coordinator.StartAtPhase(libcalib::CCoordinator::PHASE_Mag);
		m_apStep[2]->SetStatus(STATUSK_Skipped);
	}
	else
	{
		m_apStep[2]->SetStatus(STATUSK_Done);
	}

	ExpandStep(3);
}

void CWizardFrame::OnAcceptMag(wxCommandEvent & event)
{
	m_apStep[3]->SetStatus(
		g_coordinator.MagCalib().m_fitter.AreErrorsOk()
			? STATUSK_Done
			: STATUSK_Skipped);

	ExpandStep(4);
}

void CWizardFrame::OnSendCalibration(wxCommandEvent & event)
{
	auto * pMgr = Scanner().PProtomgrActive();
	if (!pMgr)
	{
		m_pTextSendStatus->SetLabel("Not connected");
		return;
	}

	libcalib::Mag::SCal cal;
	g_coordinator.GetMagCalib(&cal);
	pMgr->SendMagCal(cal);

	m_pTextSendStatus->SetLabel("Calibration sent!");
}

// --- Accordion header click ---

void CWizardFrame::OnStepHeaderClick(int iStep)
{
	// Only allow clicking on the active step (no-op) or completed/skipped steps (redo)

	if (iStep == m_iStepActive)
		return;

	STATUSK statusk = m_apStep[iStep]->Statusk();
	if (statusk != STATUSK_Done && statusk != STATUSK_Skipped)
		return;

	// Re-expanding a completed step resets the coordinator back to that phase

	// NOTE(claude) Step 0 (port) doesn't use the coordinator, steps 1-3 map to phases 0-2

	if (iStep >= 1 && iStep <= 3)
	{
		auto phase = static_cast<libcalib::CCoordinator::PHASE>(iStep - 1);
		g_coordinator.StartAtPhase(phase);

		// Mark this step and all later steps as ToDo

		for (int i = iStep; i < s_cStep; ++i)
			m_apStep[i]->SetStatus(STATUSK_ToDo);
	}

	ExpandStep(iStep);
}

// --- Timer ---

void CWizardFrame::OnTimer(wxTimerEvent & event)
{
	// NOTE(claude) MyFrame calls Scanner().Update() — we only read state here

	if (m_iStepActive < 0)
		return;

	// Call Continue() if the coordinator is in an active phase

	auto phase = g_coordinator.Phase();
	if (phase != libcalib::CCoordinator::PHASE_Nil
		&& phase != libcalib::CCoordinator::PHASE_Done)
	{
		g_coordinator.Continue();
	}

	// Update the active step's UI

	switch (m_iStepActive)
	{
	case 0:		UpdatePortStep();	break;
	case 1:		UpdateGyroStep();	break;
	case 2:		UpdateAccelStep();	break;
	case 3:		UpdateMagStep();	break;
	case 4:		UpdateSendStep();	break;
	}
}

void CWizardFrame::UpdatePortStep()
{
	#define TWEAKABLE static const
	TWEAKABLE int s_cDotsMax = 4;
	TWEAKABLE int s_msPerDot = 500;
	TWEAKABLE int s_cTicksPerDot = s_msPerDot / 14;

	if (Scanner().FIsActive())
	{
		wxString strPort = Scanner().StrPortActive();
		m_pTextPortStatus->SetLabel(
			wxString::Format("Auto-detected: %s", strPort));

		if (!m_pBtnAcceptPort->IsShown())
		{
			m_pBtnAcceptPort->Show();
			m_apStep[0]->PanelBody()->Layout();
		}
	}
	else
	{
		// Animate searching dots

		static int s_cDots = 0;
		static int s_cTicks = 0;
		if (++s_cTicks >= s_cTicksPerDot)
		{
			s_cTicks = 0;
			s_cDots = (s_cDots + 1) % (s_cDotsMax + 1);
		}
		wxString strDots(wxT('.'), s_cDots);
		m_pTextPortStatus->SetLabel(wxString::Format("Searching%s", strDots));

		if (m_pBtnAcceptPort->IsShown())
		{
			m_pBtnAcceptPort->Hide();
			m_apStep[0]->PanelBody()->Layout();
		}
	}
}

void CWizardFrame::UpdateGyroStep()
{
	auto phase = g_coordinator.Phase();

	const char * pchz = g_coordinator.PChzInstructions();
	if (pchz && phase == libcalib::CCoordinator::PHASE_Gyro)
		m_pTextGyroInstr->SetLabel(pchz);

	float uDone = g_coordinator.GyroCalib().UDone();
	m_pGaugeGyro->SetValue(int(uDone * 100.0f));

	// Auto-advance when gyro completes

	if (phase != libcalib::CCoordinator::PHASE_Gyro)
	{
		m_apStep[1]->SetStatus(STATUSK_Done);
		ExpandStep(2);
	}
}

void CWizardFrame::UpdateAccelStep()
{
	auto phase = g_coordinator.Phase();

	const char * pchz = g_coordinator.PChzInstructions();
	if (pchz && phase == libcalib::CCoordinator::PHASE_Accel)
		m_pTextAccelInstr->SetLabel(pchz);

	const auto & accelcal = g_coordinator.AccelCalib();
	int cSampFace = accelcal.CSampFace();
	float uFace = float(cSampFace) / float(libcalib::Accel::CCalibrator::s_cSampPerFace);
	m_pGaugeAccelFace->SetValue(int(uFace * 100.0f));

	float uTotal = accelcal.UDone();
	m_pGaugeAccelTotal->SetValue(int(uTotal * 100.0f));

	// Auto-advance when accel completes

	if (phase != libcalib::CCoordinator::PHASE_Accel
		&& phase != libcalib::CCoordinator::PHASE_Gyro)
	{
		m_apStep[2]->SetStatus(STATUSK_Done);
		ExpandStep(3);
	}
}

void CWizardFrame::UpdateMagStep()
{
	const auto & fitter = g_coordinator.MagCalib().m_fitter;

	m_pTextMagGaps->SetLabel(wxString::Format("%.1f%%", fitter.ErrGaps()));
	m_pTextMagVariance->SetLabel(wxString::Format("%.1f%%", fitter.ErrVariance()));
	m_pTextMagWobble->SetLabel(wxString::Format("%.1f%%", fitter.ErrWobble()));
	m_pTextMagFit->SetLabel(wxString::Format("%.1f%%", fitter.ErrFit()));

	// Auto-advance when mag quality is acceptable

	if (fitter.AreErrorsOk())
	{
		m_apStep[3]->SetStatus(STATUSK_Done);
		ExpandStep(4);
	}
}

void CWizardFrame::UpdateSendStep()
{
	// Build summary text from coordinator results

	libcalib::SPoint vecGyroBias;
	g_coordinator.GetGyroCalib(&vecGyroBias);

	libcalib::SMatrix3 matAccel;
	libcalib::SPoint vecAccelBias;
	g_coordinator.GetAccelCalib(&matAccel, &vecAccelBias);

	libcalib::Mag::SCal cal;
	g_coordinator.GetMagCalib(&cal);

	wxString strSummary;
	strSummary += wxString::Format(
		"Gyro bias: %.4f, %.4f, %.4f\n\n",
		vecGyroBias.x, vecGyroBias.y, vecGyroBias.z);

	strSummary += "Accel matrix:\n";
	for (int i = 0; i < 3; ++i)
	{
		strSummary += wxString::Format(
			"  %+.4f  %+.4f  %+.4f\n",
			matAccel[i][0], matAccel[i][1], matAccel[i][2]);
	}
	strSummary += wxString::Format(
		"Accel bias: %.4f, %.4f, %.4f\n\n",
		vecAccelBias.x, vecAccelBias.y, vecAccelBias.z);

	strSummary += wxString::Format(
		"Mag offset: %.2f, %.2f, %.2f\n",
		cal.m_vecV.x, cal.m_vecV.y, cal.m_vecV.z);

	strSummary += "Mag mapping:\n";
	for (int i = 0; i < 3; ++i)
	{
		strSummary += wxString::Format(
			"  %+.4f  %+.4f  %+.4f\n",
			cal.m_matWInv[i][0], cal.m_matWInv[i][1], cal.m_matWInv[i][2]);
	}
	strSummary += wxString::Format(
		"Field strength: %.2f uT",
		cal.m_sB);

	m_pTextSendSummary->SetLabel(strSummary);
	m_apStep[4]->PanelBody()->Layout();

	// Enable/disable send button based on connection

	m_pBtnSend->Enable(Scanner().FIsActive());
}
