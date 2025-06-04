// #include "wx/wxprec.h"
#include "mathplot.h"
#include "wx/wx.h"
#include <wx/appprogress.h>

class MainFrame : public wxFrame
{
public:
	MainFrame(const wxString &title);

	void OnQuit(wxCommandEvent &event) { Close(true); }
	void OnAbout(wxCommandEvent &event)
	{
		wxMessageBox(wxString::Format(
						 "Welcome to %s!\n"
						 "\n"
						 "This is the minimal wxWidgets sample\n"
						 "running under %s.",
						 wxGetLibraryVersionInfo().GetVersionString(),
						 wxGetOsDescription()),
					 "About wxWidgets minimal sample",
					 wxOK | wxICON_INFORMATION,
					 this);
		wxAppProgressIndicator progress(this);
		if(!progress.IsAvailable())
		{
			wxLogStatus("Progress indicator not available under this platform.");
		}
		else
		{
			wxLogStatus("Using application progress indicator...");
			progress.SetRange(10);
			for(int i = 0; i < 10; i++)
			{
				progress.SetValue(i);
				wxMilliSleep(500);
			}
			wxLogStatus("Progress finished");
		}
	}

	void bp(wxCommandEvent &event) {}

private:
	wxDECLARE_EVENT_TABLE();
};

class App : public wxApp
{
public:
	// this one is called on application startup and is a good place for the app
	// initialization (doing it here and not in the ctor allows to have an error
	// return: if OnInit() returns false, the application terminates)
	virtual bool OnInit() wxOVERRIDE
	{
		if(!wxApp::OnInit()) return false;
		MainFrame *frame = new MainFrame("Tool");
		frame->Show(true);
		return true; //  If we returned false here, the application would exit immediately
	}
};

#define bTest wxID_HIGHEST + 1
#define bText wxID_HIGHEST + 2

// the event tables connect the wxWidgets events with the functions (event handlers) which process them. It can be also done at run-time, but for the simple menu events like this the static method is much simpler.
wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_BUTTON(bTest, MainFrame::bp)
		EVT_MENU(wxID_EXIT, MainFrame::OnQuit)
			EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
				wxEND_EVENT_TABLE()

	// Create a new application object: this macro will allow wxWidgets to create the application object during program execution (it's better than using a
	// static object for many reasons) and also implements the accessor function wxGetApp() which will return the reference of the right type (i.e. app and not wxApp)
	wxIMPLEMENT_APP(App);

MainFrame::MainFrame(const wxString &title)
	// : wxFrame(NULL, wxID_ANY, title)
	// : wxFrame(NULL, -1, title, wxPoint(-1, -1), wxSize(250, 180))
	: wxFrame(NULL, wxID_ANY, title /*, wxDefaultPosition, wxSize(250, 200)*/)
{
	SetIcon(wxICON(sample));

	wxPanel *panel = new wxPanel(this, -1);

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);

	// hbox1->Add(new wxPanel(panel, -1));
	// vbox->Add(hbox1, 1, wxEXPAND);
	wxTextCtrl *text = new wxTextCtrl(panel, bText, wxT("Hi!"), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_RICH, wxDefaultValidator, wxTextCtrlNameStr);
	hbox1->Add(text, 1, wxEXPAND | wxALL, 5);
	vbox->Add(hbox1, 1, wxEXPAND);

	wxButton *ok = new wxButton(panel, -1, wxT("Ok"));
	wxButton *cancel = new wxButton(panel, -1, wxT("Cancel"));
	hbox2->Add(ok);
	hbox2->Add(cancel);

	vbox->Add(hbox2, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
	panel->SetSizer(vbox);

	Centre();

#if 1
	wxMenu *fileMenu = new wxMenu, *helpMenu = new wxMenu;
	helpMenu->Append(wxID_ABOUT, "&About\tF1", "Show about dialog");
	fileMenu->Append(wxID_EXIT, "E&xit\tAlt-X", "Quit this program");

	wxMenuBar *menuBar = new wxMenuBar();
	menuBar->Append(fileMenu, "&File");
	menuBar->Append(helpMenu, "&Help");
	SetMenuBar(menuBar);
#else // !wxUSE_MENUBAR
	// If menus are not available add a button to access the about box
	wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton *aboutBtn = new wxButton(this, wxID_ANY, "About...");
	aboutBtn->Bind(wxEVT_BUTTON, &MainFrame::OnAbout, this);
	sizer->Add(aboutBtn, wxSizerFlags().Center());
	SetSizer(sizer);
#endif
	CreateStatusBar(2);
	SetStatusText("Welcome to wxWidgets!");

	// /*wxButton *hw = */ new wxButton(this, bTest, _T("Hello World"), wxDefaultPosition, wxDefaultSize, 0 * wxBU_EXACTFIT); // with the text "hello World"
	// /*MainEditBox = */ new wxTextCtrl(this, bText, "Hi!", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_RICH, wxDefaultValidator, wxTextCtrlNameStr);
}
