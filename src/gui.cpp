#include "gui.h"
#include "imuread.h"
#include "portscanner.h"

#include "image2wx/image2wx.h"

static bool show_calibration_confirmed = false;


wxBEGIN_EVENT_TABLE(MyCanvas, wxGLCanvas)
	EVT_SIZE(MyCanvas::OnSize)
	EVT_PAINT(MyCanvas::OnPaint)
	//EVT_CHAR(MyCanvas::OnChar)
	//EVT_MOUSE_EVENTS(MyCanvas::OnMouseEvent)
wxEND_EVENT_TABLE()

MyCanvas::MyCanvas(wxWindow *parent, wxWindowID id, int* gl_attrib)
	: wxGLCanvas(parent, id, gl_attrib)
{
	//m_xrot = 0;
	//m_yrot = 0;
	//m_numverts = 0;
	// Explicitly create a new rendering context instance for this canvas.
	m_glRC = new wxGLContext(this);
}

MyCanvas::~MyCanvas()
{
	delete m_glRC;
}

void MyCanvas::OnSize(wxSizeEvent& event)
{
	//printf("OnSize\n");
	if (!IsShownOnScreen()) return;
	SetCurrent(*m_glRC);
	resize_callback(event.GetSize().x, event.GetSize().y);
}

void MyCanvas::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	//printf("OnPaint\n");
	wxPaintDC dc(this);
	SetCurrent(*m_glRC);
	display_callback();
	SwapBuffers();
}

void MyCanvas::InitGL()
{
	SetCurrent(*m_glRC);
	visualize_init();
	wxSizeEvent e = wxSizeEvent(GetSize());
	OnSize(e);
}



/*****************************************************************************/

BEGIN_EVENT_TABLE(MyFrame,wxFrame)
	EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
	EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
	EVT_MENU(ID_SENDCAL_MENU, MyFrame::OnSendCal)
	EVT_BUTTON(ID_CLEAR_BUTTON, MyFrame::OnClear)
	EVT_BUTTON(ID_SENDCAL_BUTTON, MyFrame::OnSendCal)
	EVT_TIMER(ID_TIMER, MyFrame::OnTimer)
	EVT_MENU_RANGE(9000, 9999, MyFrame::OnPortMenu)
	EVT_MENU(ID_RESTART_SCAN, MyFrame::OnPortMenu)
	EVT_MENU_OPEN(MyFrame::OnShowMenu)
END_EVENT_TABLE()

wxBitmap MyBitmap(const char *name)
{
	return image2wx::BitmapFromName(name);
}

MyFrame::MyFrame(wxWindow *parent, wxWindowID id, const wxString &title,
    const wxPoint &position, const wxSize& size, long style) :
    wxFrame( parent, id, title, position, size, style )
{
	libcalib::Calibrator & calib = libcalib::Calibrator::Ensure();

	wxPanel *panel;
	wxMenuBar *menuBar;
	wxMenu *menu;
	wxSizer *topsizer;
	wxSizer *leftsizer, *middlesizer, *rightsizer;
	wxSizer *hsizer, *vsizer, *calsizer;
	wxStaticText *text;
	int i, j;

	topsizer = new wxBoxSizer(wxHORIZONTAL);
	panel = new wxPanel(this);

	menuBar = new wxMenuBar;
	menu = new wxMenu;
	menu->Append(ID_SENDCAL_MENU, wxT("Send Calibration"));
	m_sendcal_menu = menu;
	m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
	menu->Append(wxID_EXIT, wxT("Quit"));
	menuBar->Append(menu, wxT("&File"));

	menu = new wxMenu;
	menuBar->Append(menu, "Port");
	m_port_menu = menu;

	menu = new wxMenu;
	menu->Append(wxID_ABOUT, wxT("About"));
	menuBar->Append(menu, wxT("&Help"));
	SetMenuBar(menuBar);

	leftsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Communication");
	middlesizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Magnetometer");
	rightsizer = new wxStaticBoxSizer(wxVERTICAL, panel, "Calibration");

	topsizer->Add(leftsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);
	topsizer->Add(middlesizer, 1, wxALL | wxEXPAND, 5);
	topsizer->Add(rightsizer, 0, wxALL | wxEXPAND | wxALIGN_TOP, 5);

	vsizer = new wxBoxSizer(wxVERTICAL);
	leftsizer->Add(vsizer, 0, wxALL, 8);
	text = new wxStaticText(panel, wxID_ANY, "Port");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_text_port_status = new wxStaticText(panel, wxID_ANY, "Searching...",
		wxDefaultPosition, wxDefaultSize);
	vsizer->Add(m_text_port_status, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);

	vsizer->AddSpacer(8);
	text = new wxStaticText(panel, wxID_ANY, "Actions");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_button_clear = new wxButton(panel, ID_CLEAR_BUTTON, "Clear");
	m_button_clear->Enable(false);
	vsizer->Add(m_button_clear, 1, wxEXPAND, 0);
	m_button_sendcal = new wxButton(panel, ID_SENDCAL_BUTTON, "Send Cal");
	vsizer->Add(m_button_sendcal, 1, wxEXPAND, 0);
	m_button_sendcal->Enable(false);
	vsizer->AddSpacer(16);
	text = new wxStaticText(panel, wxID_ANY, "Status");
	vsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	wxImage::AddHandler(new wxPNGHandler);
	//m_confirm_icon = new wxStaticBitmap(panel, ID_CONFIRM_ICON, MyBitmap("checkgreen.png"));
	m_confirm_icon = new wxStaticBitmap(panel, wxID_ANY, MyBitmap("checkemptygray.png"));
	vsizer->Add(m_confirm_icon, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 0);

	vsizer = new wxBoxSizer(wxVERTICAL);
	middlesizer->Add(vsizer, 1, wxEXPAND | wxALL, 8);

	text = new wxStaticText(panel, wxID_ANY, "");
	text->SetLabelMarkup("<small><i>Ideal calibration is a perfectly centered sphere</i></small>");
	vsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	int gl_attrib[20] = { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1,
		WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1, WX_GL_DOUBLEBUFFER, 0};
	m_canvas = new MyCanvas(panel, wxID_ANY, gl_attrib);
	m_canvas->SetMinSize(wxSize(480,480));
	vsizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);

	hsizer = new wxGridSizer(4, 0, 15);
	middlesizer->Add(hsizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(panel, wxID_ANY, "Gaps");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_coverage = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_coverage, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(panel, wxID_ANY, "Variance");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_variance = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_variance, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(panel, wxID_ANY, "Wobble");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_wobble = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_wobble, 1, wxALIGN_CENTER_HORIZONTAL);
	vsizer = new wxBoxSizer(wxVERTICAL);
	hsizer->Add(vsizer, 1, wxALIGN_CENTER_HORIZONTAL);
	text = new wxStaticText(panel, wxID_ANY, "Fit Error");
	vsizer->Add(text, 1, wxALIGN_CENTER_HORIZONTAL);
	m_err_fit = new wxStaticText(panel, wxID_ANY, "100.0%");
	vsizer->Add(m_err_fit, 1, wxALIGN_CENTER_HORIZONTAL);

	calsizer = new wxBoxSizer(wxVERTICAL);
	rightsizer->Add(calsizer, 0, wxALL, 8);
	text = new wxStaticText(panel, wxID_ANY, "Magnetic Offset");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_mag_offset[i] = new wxStaticText(panel, wxID_ANY, "0.00");
		vsizer->Add(m_mag_offset[i], 1);
	}
	text = new wxStaticText(panel, wxID_ANY, "Magnetic Mapping");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(3, 0, 12);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		for (j=0; j < 3; j++) {
			m_mag_mapping[i][j] = new wxStaticText(panel, wxID_ANY,
				((i == j) ? "+1.000" : "+0.000"));
			vsizer->Add(m_mag_mapping[i][j], 1);
		}
	}
	text = new wxStaticText(panel, wxID_ANY, "Magnetic Field");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	m_mag_field = new wxStaticText(panel, wxID_ANY, "0.00");
	calsizer->Add(m_mag_field, 0, wxLEFT, 20);
	text = new wxStaticText(panel, wxID_ANY, "Accelerometer");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_accel[i] = new wxStaticText(panel, wxID_ANY, "0.000");
		vsizer->Add(m_accel[i], 1);
	}
	text = new wxStaticText(panel, wxID_ANY, "Gyroscope");
	calsizer->Add(text, 0, wxTOP|wxBOTTOM, 4);
	vsizer = new wxGridSizer(1, 0, 0);
	calsizer->Add(vsizer, 1, wxLEFT, 20);
	for (i=0; i < 3; i++) {
		m_gyro[i] = new wxStaticText(panel, wxID_ANY, "0.000");
		vsizer->Add(m_gyro[i], 1);
	}

	calsizer->AddSpacer(8);
	text = new wxStaticText(panel, wxID_ANY, "");
	text->SetLabelMarkup("<small>Calibration should be performed\n<b>after</b> final installation.  Presence\nof magnets and ferrous metals\ncan alter magnetic calibration.\nMechanical stress during\nassembly can alter accelerometer\nand gyroscope calibration.</small>");
	//text->Wrap(200);
	//calsizer->Add(text, 0, wxEXPAND | wxALIGN_CENTER_HORIZONTAL, 0);
	calsizer->Add(text, 0, wxALIGN_CENTER_HORIZONTAL, 0);

	panel->SetSizer(topsizer);
	topsizer->SetSizeHints(panel);
	Fit();
	Show(true);
	Raise();

	m_canvas->InitGL();
	calib.Reset();
	//open_port(PORT);
	m_timer = new wxTimer(this, ID_TIMER);
	m_timer->Start(14, wxTIMER_CONTINUOUS);
	Scanner().StartScan();
}

void MyFrame::OnTimer(wxTimerEvent &event)
{
	#define TWEAKABLE static const
	TWEAKABLE int s_cDotsMax = 4;

	const auto & calib = libcalib::Calibrator::Ensure();
	const auto & fitter = calib.m_fitter;

	Scanner().Update();

	if (Scanner().FIsActive())
	{
		m_canvas->Refresh();

		if (fitter.AreErrorsOk())
		{
			if (!m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || !m_button_sendcal->IsEnabled())
			{
				m_sendcal_menu->Enable(ID_SENDCAL_MENU, true);
				m_button_sendcal->Enable(true);
				m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));
			}
		}
		else if (fitter.AreErrorsBad())
		{
			if (m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || m_button_sendcal->IsEnabled())
			{
				m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
				m_button_sendcal->Enable(false);
				m_confirm_icon->SetBitmap(MyBitmap("checkemptygray.png"));
			}
		}

		m_err_coverage->SetLabelText(wxString::Format("%.1f%%", fitter.ErrGaps()));
		m_err_variance->SetLabelText(wxString::Format("%.1f%%", fitter.ErrVariance()));
		m_err_wobble->SetLabelText(wxString::Format("%.1f%%", fitter.ErrWobble()));
		m_err_fit->SetLabelText(wxString::Format("%.1f%%", fitter.ErrFit()));
		for (int i=0; i < 3; i++) {
			m_mag_offset[i]->SetLabelText(wxString::Format("%.2f", fitter.m_cal.m_vecV[i]));
		}
		for (int i=0; i < 3; i++) {
			for (int j=0; j < 3; j++) {
				m_mag_mapping[i][j]->SetLabelText(wxString::Format("%+.3f", fitter.m_cal.m_matWInv[i][j]));
			}
		}
		m_mag_field->SetLabelText(wxString::Format("%.2f", fitter.m_cal.m_sB));
		for (int i=0; i < 3; i++) {
			m_accel[i]->SetLabelText(wxString::Format("%.3f", 0.0f)); // TODO...
		}
		for (int i=0; i < 3; i++) {
			m_gyro[i]->SetLabelText(wxString::Format("%.3f", 0.0f)); // TODO...
		}

		// Update status label

		m_text_port_status->SetLabelText(
			wxString::Format("IMU on %s", Scanner().StrPortActive()));

		m_button_clear->Enable(true);
	}
	else
	{
		// Not connected — scanner is probing

		// Animate the "Searching..." label with a cycling dot count
		// so the user can see the app is alive.

		TWEAKABLE int s_msPerDot = 500;
		TWEAKABLE int s_cTicksPerDot = s_msPerDot / 14;

		static int s_cDots = 0;
		static int s_cTicks = 0;
		if (++s_cTicks >= s_cTicksPerDot)
		{
			s_cTicks = 0;
			s_cDots = (s_cDots + 1) % (s_cDotsMax + 1);
		}
		wxString strDots(wxT('.'), s_cDots);
		m_text_port_status->SetLabelText(wxString::Format("Searching%s", strDots));

		// Disable controls that require a connection

		if (m_sendcal_menu->IsEnabled(ID_SENDCAL_MENU) || m_button_sendcal->IsEnabled())
		{
			m_sendcal_menu->Enable(ID_SENDCAL_MENU, false);
			m_button_sendcal->Enable(false);
			m_button_clear->Enable(false);
			m_confirm_icon->SetBitmap(MyBitmap("checkemptygray.png"));
		}
	}

	if (show_calibration_confirmed)
	{
		m_confirm_icon->SetBitmap(MyBitmap("checkgreen.png"));
		show_calibration_confirmed = false;
	}
}

void MyFrame::OnClear(wxCommandEvent &event)
{
	libcalib::Calibrator & calib = libcalib::Calibrator::Ensure();

	//printf("OnClear\n");
	calib.Reset();
}

void MyFrame::OnSendCal(wxCommandEvent &event)
{
	//const auto & calib = libcalib::Calibrator::Ensure();
	//const auto & fitter = calib.m_fitter;


	//printf("OnSendCal\n");
	//const auto & V = fitter.m_cal.m_vecV;
	//const auto & invW = fitter.m_cal.m_matWInv;
	//const auto & errFit = fitter.m_errFit;
	//printf("Magnetic Calibration:   (%.1f%% fit error)\n", errFit);
	//printf("   %7.2f   %6.3f %6.3f %6.3f\n",
	//	V[0], invW[0][0], invW[0][1], invW[0][2]);
	//printf("   %7.2f   %6.3f %6.3f %6.3f\n",
	//	V[1], invW[1][0], invW[1][1], invW[1][2]);
	//printf("   %7.2f   %6.3f %6.3f %6.3f\n",
	//	V[2], invW[2][0], invW[2][1], invW[2][2]);
	
	m_confirm_icon->SetBitmap(MyBitmap("checkempty.png"));
	send_calibration();
}

void calibration_confirmed()
{
	show_calibration_confirmed = true;
}


void MyFrame::OnShowMenu(wxMenuEvent &event)
{
	wxMenu *menu = event.GetMenu();
	if (menu != m_port_menu) return;

	while (menu->GetMenuItemCount() > 0)
	{
		menu->Delete(menu->GetMenuItems()[0]);
	}

	menu->AppendRadioItem(9000, " (none)");
	if (!Scanner().FIsActive()) menu->Check(9000, true);

	wxArrayString list = serial_port_list();
	int num = list.GetCount();
	for (int i=0; i < num; i++)
	{
		menu->AppendRadioItem(9001 + i, list[i]);
		if (Scanner().FIsActive() && Scanner().StrPortActive().IsSameAs(list[i]))
		{
			menu->Check(9001 + i, true);
		}
	}

	menu->AppendSeparator();
	menu->Append(ID_RESTART_SCAN, wxT("Restart Scan"));

	menu->UpdateUI();
}

void MyFrame::OnPortMenu(wxCommandEvent &event)
{
	libcalib::Calibrator & calib = libcalib::Calibrator::Ensure();

	int id = event.GetId();
	if (id == ID_RESTART_SCAN)
	{
		Scanner().RestartScan();
		calib.Reset();
		m_button_clear->Enable(false);
		return;
	}

	wxString strName = m_port_menu->FindItem(id)->GetItemLabelText();
	if (id == 9000)
	{
		// (none) — stop scanning, disconnect

		Scanner().RestartScan();
		calib.Reset();
		m_button_clear->Enable(false);
		return;
	}

	calib.Reset();
	Scanner().ForcePort(strName);
	m_button_clear->Enable(Scanner().FIsActive());
}




void MyFrame::OnAbout(wxCommandEvent &event)
{
        wxMessageDialog dialog(this,
                "SensorCal - Motion Sensor Calibration Tool\n"
				"\n"
				"Bruce Oberg <bruce@oberg.org>\n"
				"https://github.com/bruceoberg/SensorCal\n"
				"SensorCal Copyright 2023, Bruce Oberg\n"
				"\n"
				"Based on MotionCal by Paul Stoffregen <paul@pjrc.com>\n"
				"http://www.pjrc.com/store/prop_shield.html\n"
				"https://github.com/PaulStoffregen/MotionCal\n"
				"MotionCal Copyright 2018, PJRC.COM, LLC.",
                "About SensorCal", wxOK|wxICON_INFORMATION|wxCENTER);
        dialog.ShowModal();
}

void MyFrame::OnQuit( wxCommandEvent &event )
{
        Close(true);
}

MyFrame::~MyFrame()
{
	m_timer->Stop();
	delete m_timer;
	Scanner().CloseActive();
}


/*****************************************************************************/

IMPLEMENT_APP(MyApp)

MyApp::MyApp()
{
}

bool MyApp::OnInit()
{
	// make sure we exit properly on macosx
	SetExitOnFrameDelete(true);

	wxPoint pos(100, 100);

	MyFrame *frame = new MyFrame(NULL, -1, "Motion Sensor Calibration Tool",
		pos, wxSize(1120,760), wxDEFAULT_FRAME_STYLE);
#ifdef WINDOWS
	frame->SetIcon(wxIcon("SensorCal"));
#endif
	frame->Show( true );
	return true;
}

int MyApp::OnExit()
{
	image2wx::FreeBitmaps();
	return 0;
}




