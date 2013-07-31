#include <wx/wx.h>
#include <wx/intl.h>
#include <wx/thread.h>
#include <wx/timer.h>
#include <wx/datetime.h>
#include <wx/filepicker.h>
#include <wx/utils.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/spinctrl.h>
#include <wx/clipbrd.h>
#include <wx/url.h>

#ifdef __WXGTK__
  #include <curl/curl.h>
#elif __WXMSW__
  #include <windows.h>
  #include <wininet.h>
#endif

#include <memory>

static const wxString ua("Mozilla/5.0 (X11; Linux x86_64; rv:21.0) Gecko/20100101 Firefox/21.0");

enum {
	ID_NONE = 6000,
	ID_BUTTON_COPY,
	ID_BUTTON_CHECK
};

#ifdef __WXGTK__
static size_t HttpGet_WriteStr(void *buffer, size_t size, size_t nmemb, void *userp)
{
	const size_t realsize = size * nmemb;
	char *str = (char *)buffer;

	wxString *text = reinterpret_cast<wxString *>(userp);
	text->append(str, realsize);

	return realsize;
}

static void HttpGet(const wxString& url, std::shared_ptr<wxString>& str)
{
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url.ToStdString().c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpGet_WriteStr);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<wxString *>(str.get()));

	curl_easy_setopt(curl, CURLOPT_USERAGENT, ua.ToStdString().c_str());
	//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10 sek

	CURLcode result = curl_easy_perform(curl);

	if (result != CURLE_OK) {
		//wxLogError("Błąd połączenia z URL %s (%s)", url.c_str(), curl_easy_strerror(result));
	}

	curl_easy_cleanup(curl);
}
#elif __WXMSW__
static void HttpGet(const wxString& url, std::shared_ptr<wxString>& str)
{
	wxString host(url), req;

	// Usuń z adresu http[s]://
	if (host.substr(0, 4) == "http") {
		host = host.substr(7);
	}

	// Wyciągnij wszystko po pierwszym '/'
	const size_t idx = host.find("/");
	if (idx != wxString::npos) {
		req = host.substr(idx);
		host = host.substr(0, idx);
	}

	HINTERNET hInternet = InternetOpenW(ua.wc_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);

	if (hInternet) {
		HINTERNET hConnect = InternetConnectW(hInternet, host.wc_str(), 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

		if (hConnect) {
			const wchar_t* parrAcceptTypes[] = { L"text/*", NULL };
			HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", req.wc_str(), NULL, NULL, parrAcceptTypes, 0, 0);

			if (hRequest) {
				BOOL bRequestSent = HttpSendRequestW(hRequest, NULL, 0, NULL, 0);

				if (bRequestSent) {
					const int nBuffSize = 10240; // 10 kB
					char buff[nBuffSize];

					BOOL bKeepReading = true;
					DWORD dwBytesRead = -1;

					while (bKeepReading && dwBytesRead != 0) {
						bKeepReading = InternetReadFile(hRequest, buff, nBuffSize, &dwBytesRead);
						str->append(buff, dwBytesRead);
					}
				}

				InternetCloseHandle(hRequest);
			}

			InternetCloseHandle(hConnect);
		}

		InternetCloseHandle(hInternet);
	}
}
#endif

class MainWindow : public wxFrame, public wxThreadHelper
{
	wxDECLARE_EVENT_TABLE();

public:
	MainWindow()
		: wxFrame(NULL, wxID_ANY, "IPup", wxDefaultPosition, wxSize(350, 400)),
		  m_timer(this)
	{
#ifdef __WXMSW__
		SetIcon(wxIcon(wxICON(ipup)));
#endif
		wxPanel *mainPanel = new wxPanel(this);
		wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

		{
			wxStaticBoxSizer* sbs = new wxStaticBoxSizer(new wxStaticBox(mainPanel, wxID_ANY, wxT("Gdzie zapisać aktualne IP")), wxHORIZONTAL);

			const wxFileName fn(wxGetCwd(), "IP.txt");
			m_ipDestFile = new wxFilePickerCtrl(mainPanel, wxID_ANY, fn.GetFullPath());

			sbs->Add(m_ipDestFile, 1, wxEXPAND|wxALL, 5);
			mainSizer->Add(sbs, 0, wxEXPAND|wxALL, 5);
		}

		{
			wxStaticBoxSizer* sbs = new wxStaticBoxSizer(new wxStaticBox(mainPanel, wxID_ANY, wxT("Adres strony do pobrania IP")), wxVERTICAL);

			m_ipProvider = new wxComboBox(mainPanel, wxID_ANY);
			m_ipProvider->Append("http://ifconfig.me/ip");
			m_ipProvider->Append("http://checkip.dyndns.com/");
			m_ipProvider->Append("http://showip.net/");
			m_ipProvider->Append("http://bot.whatismyipaddress.com/");
			m_ipProvider->SetSelection(1);

			sbs->Add(m_ipProvider, 1, wxEXPAND|wxALL, 5);
			mainSizer->Add(sbs, 0, wxEXPAND|wxALL, 5);
		}

		{
			wxStaticBoxSizer* sbs = new wxStaticBoxSizer(new wxStaticBox(mainPanel, wxID_ANY, wxT("Wyrażenie do wyciągania IP")), wxVERTICAL);

			m_ipRegex = new wxTextCtrl(mainPanel, wxID_ANY, "\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");

			sbs->Add(m_ipRegex, 1, wxEXPAND|wxALL, 5);
			mainSizer->Add(sbs, 0, wxEXPAND|wxALL, 5);
		}

		{
			wxStaticBoxSizer* sbs = new wxStaticBoxSizer(new wxStaticBox(mainPanel, wxID_ANY, "Czas sprawdzania IP"), wxHORIZONTAL);

			m_minInterval = new wxSpinCtrl(mainPanel, wxID_ANY, "60", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1440, 60);
			m_minInterval->SetToolTip(wxT("Aby zastosować zmiany kliknij przycisk \"Sprawdź\""));

			sbs->Add(new wxStaticText(mainPanel, wxID_ANY, wxT("Sprawdź IP co")), 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
			sbs->Add(m_minInterval, 1, wxEXPAND|wxALL, 5);
			sbs->Add(new wxStaticText(mainPanel, wxID_ANY, "minut"), 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);

			mainSizer->Add(sbs, 0, wxEXPAND|wxALL, 5);
		}

		{
			wxStaticBoxSizer* sbs = new wxStaticBoxSizer(new wxStaticBox(mainPanel, wxID_ANY, "Aktualne IP"), wxHORIZONTAL);

			m_ip = new wxTextCtrl(mainPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
			wxButton *copy = new wxButton(mainPanel, ID_BUTTON_COPY, wxT("Kopiuj"));
			wxButton *check = new wxButton(mainPanel, ID_BUTTON_CHECK, wxT("Sprawdź"));

			sbs->Add(copy, 0, wxEXPAND|wxALL, 5);
			sbs->Add(m_ip, 1, wxEXPAND|wxALL, 5);
			sbs->Add(check, 0, wxEXPAND|wxALL, 5);

			mainSizer->Add(sbs, 0, wxEXPAND|wxALL, 5);
		}

		CreateStatusBar(1, wxSTB_DEFAULT_STYLE, -1);
		SetStatusText("IPup v0.1");

		mainPanel->SetSizerAndFit(mainSizer);

		const int HOUR = 60 * (1000 * 60);
		m_timer.Start(HOUR, wxTIMER_CONTINUOUS);
	}

	void OnCopy(wxCommandEvent &event)
	{
		if (!m_ip->GetValue().IsEmpty() && wxTheClipboard->Open()) {
			// This data objects are held by the clipboard, so do not delete them in the app.
			wxTheClipboard->SetData(new wxTextDataObject(m_ip->GetValue()));
			wxTheClipboard->Close();
		}
	}

	void OnCheck(wxCommandEvent &event)
	{
		const int interval_min = m_timer.GetInterval() / (1000 * 60);

		if (interval_min != m_minInterval->GetValue()) {
			const int new_interval = m_minInterval->GetValue() * (1000 * 60);
			m_timer.Start(new_interval, wxTIMER_CONTINUOUS);
		}

		const wxString url = m_ipProvider->GetValue();

		CheckIpAsync(url);
	}

	void OnTimer(wxTimerEvent &event)
	{
		const wxString url = m_ipProvider->GetValue();

		if (wxURL(url).GetError() != wxURL_NOERR) {
			SetStatusText(wxT("Nieprawidłowy URL"));
			return;
		}

		CheckIpAsync(url);
	}

	wxThread::ExitCode Entry()
	{
		CallAfter(&MainWindow::SetStatusText, wxString("Sprawdzanie adresu IP..."), 0);

		std::shared_ptr<wxString> buff(new wxString);

		HttpGet(m_urlStr, buff);

		buff->Trim(false).Trim(true);

		CallAfter(&MainWindow::SetIP, *buff);

		return static_cast<wxThread::ExitCode>(0);
	}

private:
	void SetIP(const wxString& buff)
	{
		wxRegEx re(m_ipRegex->GetValue(), wxRE_ADVANCED);

		if (!re.Matches(buff) || re.GetMatch(buff).IsEmpty()) {
			SetStatusText(wxT("Błąd parsowania IP"));
			return;
		}

		const wxString ip(re.GetMatch(buff));

		m_ip->SetValue(ip);

		m_lastIpTime = wxDateTime::Now();
		SetStatusText("IP: " + ip + " z godziny " +  m_lastIpTime.Format("%H:%M (%S sek)"));

		const wxFileName fn(m_ipDestFile->GetPath());
		wxFile ipFile;

		if (!ipFile.Create(fn.GetFullPath(), true)) {
			wxLogError(wxT("Błąd podczas tworzenia pliku"));
			return;
		}

		ipFile.Write(ip + "\n" + wxNow() + "\n");
	}

	void CheckIpAsync(const wxString& url)
	{
		m_urlStr = url;

		if (CreateThread(wxTHREAD_JOINABLE) != wxTHREAD_NO_ERROR) {
			SetStatusText(wxT("Błąd, nie można utworzyć wątku roboczego"));
			return;
		}

		if (GetThread()->Run() != wxTHREAD_NO_ERROR) {
			SetStatusText(wxT("Błąd, nie można uruchomić wątku"));
			return;
		}
	}

private:
	wxFilePickerCtrl *m_ipDestFile;
	wxComboBox *m_ipProvider;
	wxTextCtrl *m_ipRegex;
	wxTextCtrl *m_ip;
	wxSpinCtrl *m_minInterval;
	wxTimer m_timer;
	wxDateTime m_lastIpTime;
	wxString m_urlStr;
};

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_BUTTON(ID_BUTTON_COPY, MainWindow::OnCopy)
	EVT_BUTTON(ID_BUTTON_CHECK, MainWindow::OnCheck)
	EVT_TIMER(wxID_ANY, MainWindow::OnTimer)
wxEND_EVENT_TABLE()

class App : public wxApp
{
public:
	bool OnInit()
	{
		wxLocale locale(wxLANGUAGE_DEFAULT);

		MainWindow *window = new MainWindow;
		window->Show();
		window->Center();

		return true;
	}
};

wxIMPLEMENT_APP(App);
