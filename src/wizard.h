#ifndef wizard_h_
#define wizard_h_

#include <wx/wx.h>
#include <wx/timer.h>

// Step status for badge display

enum STATUSK
{
	STATUSK_ToDo,
	STATUSK_Active,
	STATUSK_Done,
	STATUSK_Skipped,

	STATUSK_Max,
	STATUSK_Min = 0,
	STATUSK_Nil = -1,
};

// One collapsible step card in the wizard accordion

class CStepCard : public wxPanel	// tag = step
{
public:
	CStepCard(
		wxWindow * pParent,
		int nStep,						// 1-based step number
		const wxString & strName);		// step name

	wxPanel * PanelBody()				{ return m_pPanelBody; }

	void Expand();
	void Collapse();
	bool FIsExpanded() const			{ return m_fExpanded; }

	void SetStatus(STATUSK statusk);
	STATUSK Statusk() const				{ return m_statusk; }

private:
	void OnHeaderClick(wxMouseEvent & event);

	wxPanel * m_pPanelHeader;
	wxPanel * m_pPanelBody;
	wxStaticText * m_pTextNumber;		// "1."
	wxStaticText * m_pTextName;			// step name
	wxStaticText * m_pTextStatus;		// badge: "To Do" / "Active" / "Done" / "Skipped"
	bool m_fExpanded;
	STATUSK m_statusk;
};

// The wizard window — drives CCoordinator through five calibration steps

class CWizardFrame : public wxFrame	// tag = wiz
{
public:
	CWizardFrame(
		wxWindow * pParent,
		wxWindowID id,
		const wxString & strTitle,
		const wxPoint & pos,
		const wxSize & size);

	~CWizardFrame();

private:
	// Accordion

	void ExpandStep(int iStep);			// 0-based; collapses current, expands new
	void OnStepHeaderClick(int iStep);	// handle click on a step header

	// Step body builders — called once during construction

	void BuildPortBody(wxPanel * pBody);
	void BuildGyroBody(wxPanel * pBody);
	void BuildAccelBody(wxPanel * pBody);
	void BuildMagBody(wxPanel * pBody);
	void BuildSendBody(wxPanel * pBody);

	// Button handlers

	void OnAcceptPort(wxCommandEvent & event);
	void OnAcceptGyro(wxCommandEvent & event);
	void OnAcceptAccel(wxCommandEvent & event);
	void OnAcceptMag(wxCommandEvent & event);
	void OnSendCalibration(wxCommandEvent & event);

	// Timer

	void OnTimer(wxTimerEvent & event);
	void UpdatePortStep();
	void UpdateGyroStep();
	void UpdateAccelStep();
	void UpdateMagStep();
	void UpdateSendStep();

	// Step cards

	static const int s_cStep = 5;
	CStepCard * m_apStep[s_cStep];
	int m_iStepActive;					// 0-based, or -1

	wxTimer * m_timer;

	// Step 1: Choose Port

	wxStaticText * m_pTextPortStatus;
	wxButton * m_pBtnAcceptPort;

	// Step 2: Gyro

	wxStaticText * m_pTextGyroInstr;
	wxGauge * m_pGaugeGyro;
	wxButton * m_pBtnAcceptGyro;

	// Step 3: Accel

	wxStaticText * m_pTextAccelInstr;
	wxGauge * m_pGaugeAccelFace;		// per-face progress
	wxGauge * m_pGaugeAccelTotal;		// overall progress
	wxButton * m_pBtnAcceptAccel;

	// Step 4: Mag

	wxStaticText * m_pTextMagInstr;
	wxStaticText * m_pTextMagGaps;
	wxStaticText * m_pTextMagVariance;
	wxStaticText * m_pTextMagWobble;
	wxStaticText * m_pTextMagFit;
	wxButton * m_pBtnAcceptMag;

	// Step 5: Send

	wxStaticText * m_pTextSendSummary;
	wxButton * m_pBtnSend;
	wxStaticText * m_pTextSendStatus;

	DECLARE_EVENT_TABLE()
};

#endif // wizard_h_
