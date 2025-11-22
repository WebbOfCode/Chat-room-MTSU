// user3_gui.cpp - Second GUI chat client (same behavior as user2_gui). Simplified.

#include <wx/wx.h>
#include <wx/socket.h>

using namespace std; // requested

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

class ClientFrame : public wxFrame {
public:
    ClientFrame(const wxString& title);
    ~ClientFrame();

private:
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnSendMessage(wxCommandEvent& event);
    void OnSocketEvent(wxSocketEvent& event);
    
    void ConnectToServer(const wxString& host, int port);
    void DisconnectFromServer();
    void SendMessage(const wxString& message);
    void LogMessage(const wxString& message);
    
    wxTextCtrl* m_chatDisplay;
    wxTextCtrl* m_messageInput;
    wxButton* m_sendButton;
    wxTextCtrl* m_hostInput;
    wxTextCtrl* m_portInput;
    wxButton* m_connectButton;
    wxButton* m_disconnectButton;
    
    wxSocketClient* m_socket;
    bool m_connected;
    
    wxDECLARE_EVENT_TABLE();
};

enum {
    ID_Quit = 1,
    ID_About,
    ID_Connect,
    ID_Disconnect,
    ID_Send,
    SOCKET_ID
};

wxBEGIN_EVENT_TABLE(ClientFrame, wxFrame)
    EVT_MENU(ID_Quit, ClientFrame::OnQuit)
    EVT_MENU(ID_About, ClientFrame::OnAbout)
    EVT_BUTTON(ID_Connect, ClientFrame::OnConnect)
    EVT_BUTTON(ID_Disconnect, ClientFrame::OnDisconnect)
    EVT_BUTTON(ID_Send, ClientFrame::OnSendMessage)
    EVT_SOCKET(SOCKET_ID, ClientFrame::OnSocketEvent)
wxEND_EVENT_TABLE()

ClientFrame::ClientFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(700, 500)),
      m_socket(nullptr), m_connected(false) {
    
    // Create menu bar
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(ID_About, "&About\tF1", "Show about dialog");
    menuFile->AppendSeparator();
    menuFile->Append(ID_Quit, "E&xit\tAlt-X", "Quit this program");
    
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);
    
    CreateStatusBar(2);
    SetStatusText("Not connected");
    
    // Create main panel
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Connection controls
    wxStaticBoxSizer* connectionBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, "Connection");
    
    connectionBox->Add(new wxStaticText(panel, wxID_ANY, "Host:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_hostInput = new wxTextCtrl(panel, wxID_ANY, "127.0.0.1", wxDefaultPosition, wxSize(120, -1));
    connectionBox->Add(m_hostInput, 0, wxALL, 5);
    
    connectionBox->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_portInput = new wxTextCtrl(panel, wxID_ANY, "8888", wxDefaultPosition, wxSize(60, -1));
    connectionBox->Add(m_portInput, 0, wxALL, 5);
    
    m_connectButton = new wxButton(panel, ID_Connect, "Connect");
    connectionBox->Add(m_connectButton, 0, wxALL, 5);
    
    m_disconnectButton = new wxButton(panel, ID_Disconnect, "Disconnect");
    m_disconnectButton->Enable(false);
    connectionBox->Add(m_disconnectButton, 0, wxALL, 5);
    
    mainSizer->Add(connectionBox, 0, wxALL | wxEXPAND, 5);
    
    // Chat display
    m_chatDisplay = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    mainSizer->Add(m_chatDisplay, 1, wxALL | wxEXPAND, 5);
    
    // Message input area
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    
    inputSizer->Add(new wxStaticText(panel, wxID_ANY, "Message:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    m_messageInput = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxTE_PROCESS_ENTER);
    m_messageInput->Bind(wxEVT_TEXT_ENTER, &ClientFrame::OnSendMessage, this);
    m_messageInput->Enable(false);
    inputSizer->Add(m_messageInput, 1, wxALL, 5);
    
    m_sendButton = new wxButton(panel, ID_Send, "Send");
    m_sendButton->Enable(false);
    inputSizer->Add(m_sendButton, 0, wxALL, 5);
    
    mainSizer->Add(inputSizer, 0, wxEXPAND);
    
    panel->SetSizer(mainSizer);
    
    LogMessage("Welcome to User3 Chat Client!");
    LogMessage("Enter server host and port, then click Connect.");
}

ClientFrame::~ClientFrame() {
    if (m_socket) {
        m_socket->Destroy();
        m_socket = nullptr;
    }
}

void ClientFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    Close(true);
}

void ClientFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    wxMessageBox("Multi-User Chat Client (User3)\n\n"
                 "Connect to User1 server to chat.",
                 "About User3 Chat Client",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void ClientFrame::OnConnect(wxCommandEvent& WXUNUSED(event)) {
    wxString host = m_hostInput->GetValue().Trim();
    long port;
    
    if (host.IsEmpty()) {
        wxMessageBox("Please enter a host address", "Error", wxICON_ERROR);
        return;
    }
    
    if (!m_portInput->GetValue().ToLong(&port) || port <= 0 || port > 65535) {
        wxMessageBox("Please enter a valid port number (1-65535)", "Error", wxICON_ERROR);
        return;
    }
    
    ConnectToServer(host, static_cast<int>(port));
}

void ClientFrame::OnDisconnect(wxCommandEvent& WXUNUSED(event)) {
    DisconnectFromServer();
}

void ClientFrame::OnSendMessage(wxCommandEvent& WXUNUSED(event)) {
    if (!m_connected) {
        LogMessage("Not connected to server!");
        return;
    }
    
    wxString message = m_messageInput->GetValue().Trim();
    if (message.IsEmpty()) {
        return;
    }
    
    SendMessage(message);
    LogMessage("[You] " + message);
    m_messageInput->Clear();
}

void ClientFrame::OnSocketEvent(wxSocketEvent& event) {
    switch (event.GetSocketEvent()) {
        case wxSOCKET_INPUT: {
            char buffer[1024];
            m_socket->Read(buffer, sizeof(buffer) - 1);
            wxUint32 len = m_socket->LastCount();
            
            if (len > 0) {
                buffer[len] = '\0';
                wxString message(buffer, wxConvUTF8, len);
                message.Trim();
                
                if (!message.IsEmpty()) {
                    LogMessage("[Server] " + message);
                }
            }
            break;
        }
        
        case wxSOCKET_LOST:
            LogMessage("Connection lost!");
            DisconnectFromServer();
            wxMessageBox("Connection to server lost", "Disconnected", wxICON_WARNING);
            break;
            
        case wxSOCKET_CONNECTION:
            LogMessage("Connected to server!");
            m_connected = true;
            SetStatusText("Connected", 1);
            
            m_connectButton->Enable(false);
            m_disconnectButton->Enable(true);
            m_hostInput->Enable(false);
            m_portInput->Enable(false);
            m_messageInput->Enable(true);
            m_sendButton->Enable(true);
            break;
            
        default:
            break;
    }
}

void ClientFrame::ConnectToServer(const wxString& host, int port) {
    if (m_connected) {
        wxMessageBox("Already connected to a server", "Warning", wxICON_WARNING);
        return;
    }
    
    LogMessage("Connecting to " + host + ":" + wxString::Format("%d", port) + "...");
    
    // Create socket
    m_socket = new wxSocketClient();
    m_socket->SetEventHandler(*this, SOCKET_ID);
    m_socket->SetNotify(wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    m_socket->Notify(true);
    
    // Setup address
    wxIPV4address addr;
    addr.Hostname(host);
    addr.Service(port);
    
    // Connect (non-blocking)
    m_socket->Connect(addr, false);
    
    // Wait for connection
    if (!m_socket->WaitOnConnect(10)) {
        LogMessage("Connection failed!");
        wxMessageBox("Failed to connect to server", "Connection Error", wxICON_ERROR);
        m_socket->Destroy();
        m_socket = nullptr;
        return;
    }
}

void ClientFrame::DisconnectFromServer() {
    if (m_socket) {
        m_socket->Close();
        m_socket->Destroy();
        m_socket = nullptr;
    }
    
    m_connected = false;
    SetStatusText("Not connected", 1);
    
    m_connectButton->Enable(true);
    m_disconnectButton->Enable(false);
    m_hostInput->Enable(true);
    m_portInput->Enable(true);
    m_messageInput->Enable(false);
    m_sendButton->Enable(false);
    
    LogMessage("Disconnected from server.");
}

void ClientFrame::SendMessage(const wxString& message) {
    if (!m_connected || !m_socket) {
        return;
    }
    
    wxString msg = message + "\n";
    m_socket->Write(msg.mb_str(), msg.length());
}

void ClientFrame::LogMessage(const wxString& message) {
    m_chatDisplay->AppendText(message + "\n");
}

// Application class
class ClientApp : public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();
};

bool ClientApp::OnInit() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        wxMessageBox("Failed to initialize Winsock", "Error", wxICON_ERROR);
        return false;
    }
#endif

    ClientFrame* frame = new ClientFrame("User3 Chat Client");
    frame->Show(true);
    return true;
}

int ClientApp::OnExit() {
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

wxIMPLEMENT_APP(ClientApp);
