// user2_gui.cpp
// simple multi-user chat client


#include <wx/wx.h>        // wx gui
#include <wx/socket.h>    // tcp socket

//platform net stuff (winsock vs posix) for windows and linux
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

// main client window (connect, type, chat)
class ClientFrame : public wxFrame {
public:
    ClientFrame(const wxString& title);
    ~ClientFrame();

private:
    // ui events
    void OnQuit(wxCommandEvent& event);           // exit app
        void OnAbout(wxCommandEvent& event);          //about box
    void OnConnect(wxCommandEvent& event);        //connect btn
  void OnDisconnect(wxCommandEvent& event);     //disconnect btn
    void OnSendMessage(wxCommandEvent& event);    //send / enter
    void OnSocketEvent(wxSocketEvent& event);     //net events
    
    // helpers functions
    void ConnectToServer(const wxString& host, int port);  // open conn
    void DisconnectFromServer();                           // close conn
        void SendMessage(const wxString& message);             // push msg to srv
    void LogMessage(const wxString& message);              // print in chat
    
    // ui bits
    wxTextCtrl* m_chatDisplay;       // chat log
        wxTextCtrl* m_messageInput;      // where we type
    wxButton*   m_sendButton;          // send btn
  wxTextCtrl* m_hostInput;         // server ip
    wxTextCtrl* m_portInput;         // server port
    wxButton*   m_connectButton;       // connect
        wxButton* m_disconnectButton;   // disconnect
    
    // net state
    wxSocketClient* m_socket;        //active socket
    bool            m_connected;     //its connected
    
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
    EVT_MENU(ID_Quit,        ClientFrame::OnQuit)
    EVT_MENU(ID_About,       ClientFrame::OnAbout)
    EVT_BUTTON(ID_Connect,   ClientFrame::OnConnect)
  EVT_BUTTON(ID_Disconnect, ClientFrame::OnDisconnect)
    EVT_BUTTON(ID_Send,      ClientFrame::OnSendMessage)
    EVT_SOCKET(SOCKET_ID,    ClientFrame::OnSocketEvent)
wxEND_EVENT_TABLE()

ClientFrame::ClientFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(700, 500)),
      m_socket(nullptr),
      m_connected(false)
{
    //menu bar
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(ID_About, "&About\tF1", "what is this thing");
    menuFile->AppendSeparator();
    menuFile->Append(ID_Quit, "E&xit\tAlt-X", "quit app");
    
    wxMenuBar* menuBar = new wxMenuBar();
        menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);
    
    CreateStatusBar(2);
    SetStatusText("not connected");
    
    //gui layout
    wxPanel*    panel     = new wxPanel(this, wxID_ANY);
  wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    //the row to connect and disconnect
    wxStaticBoxSizer* connectionBox =
        new wxStaticBoxSizer(wxHORIZONTAL, panel, "connection");
    
    connectionBox->Add(new wxStaticText(panel, wxID_ANY, "host:"),
                       0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_hostInput = new wxTextCtrl(panel, wxID_ANY, "127.0.0.1",
                                 wxDefaultPosition, wxSize(120, -1));
    connectionBox->Add(m_hostInput, 0, wxALL, 5);
    
        connectionBox->Add(new wxStaticText(panel, wxID_ANY, "port:"),
                       0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    m_portInput = new wxTextCtrl(panel, wxID_ANY, "8888",
                                 wxDefaultPosition, wxSize(60, -1));
    connectionBox->Add(m_portInput, 0, wxALL, 5);
    
    m_connectButton = new wxButton(panel, ID_Connect, "connect");
    connectionBox->Add(m_connectButton, 0, wxALL, 5);
    
        m_disconnectButton = new wxButton(panel, ID_Disconnect, "disconnect");
    m_disconnectButton->Enable(false);
    connectionBox->Add(m_disconnectButton, 0, wxALL, 5);
    
    mainSizer->Add(connectionBox, 0, wxALL | wxEXPAND, 5);
    
    //this is the chat box
    m_chatDisplay = new wxTextCtrl(
        panel,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH
    );
    mainSizer->Add(m_chatDisplay, 1, wxALL | wxEXPAND, 5);
    
    //input to type messages 
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    
    inputSizer->Add(new wxStaticText(panel, wxID_ANY, "msg:"),
                    0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
        m_messageInput = new wxTextCtrl(
        panel,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_PROCESS_ENTER
    );
    m_messageInput->Bind(wxEVT_TEXT_ENTER, &ClientFrame::OnSendMessage, this);
    m_messageInput->Enable(false);
    inputSizer->Add(m_messageInput, 1, wxALL, 5);
    
    m_sendButton = new wxButton(panel, ID_Send, "send");
        m_sendButton->Enable(false);
    inputSizer->Add(m_sendButton, 0, wxALL, 5);
    
    mainSizer->Add(inputSizer, 0, wxEXPAND);
    
    panel->SetSizer(mainSizer);
    
    LogMessage("user2 chat client ready");
        LogMessage("set host/port and hit connect");
}

ClientFrame::~ClientFrame() {
    if (m_socket) {
        m_socket->Destroy();
        m_socket = nullptr;
    }
}

void ClientFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    Close(true);   //this closes the app
}

void ClientFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    wxMessageBox(
        "multi-user chat client (user2)\n\n"
        "connects to user1 server.",
        "about user2",
        wxOK | wxICON_INFORMATION,
        this
    );
}

void ClientFrame::OnConnect(wxCommandEvent& WXUNUSED(event)) {
    wxString host = m_hostInput->GetValue().Trim();
    long port;
    
    if (host.IsEmpty()) {
        wxMessageBox("need a host", "Error", wxICON_ERROR);
        return;
    }
    
    if (!m_portInput->GetValue().ToLong(&port) || port <= 0 || port > 65535) {
        wxMessageBox("bad port (1-65535)", "Error", wxICON_ERROR);
        return;
    }
    
    ConnectToServer(host, static_cast<int>(port));
}

void ClientFrame::OnDisconnect(wxCommandEvent& WXUNUSED(event)) {
        DisconnectFromServer();
}

void ClientFrame::OnSendMessage(wxCommandEvent& WXUNUSED(event)) {
    if (!m_connected) {
        LogMessage("not connected to server");
        return;
    }
    
    wxString message = m_messageInput->GetValue().Trim();
    if (message.IsEmpty()) {
        return;   //no empty messages
    }
    
    // send the message to the server.
    // if it's "Exit", the server will send "Exit" back,
    // and we'll handle that in OnSocketEvent.
    SendMessage(message);    //server echos to everyone
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
                message.Trim(true).Trim(false);

                if (!message.IsEmpty()) {
                    // if server sends "Exit", close the connection
                    if (message == "Exit") {
                        LogMessage("server sent Exit - closing connection");
                        DisconnectFromServer();
                    } else {
                        LogMessage(message);   //prints to teh chat display
                    }
                }
            }
            break;
        }
        
        case wxSOCKET_LOST:
            LogMessage("connection lost :(");
            DisconnectFromServer();
            wxMessageBox("server disconnected", "Disconnected", wxICON_WARNING);
            break;
            
        case wxSOCKET_CONNECTION:
            LogMessage("connected to server");
            m_connected = true;
            SetStatusText("connected", 1);
            
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
        wxMessageBox("already connected", "Warning", wxICON_WARNING);
        return;
    }
    
    LogMessage("connecting to " + host + ":" + wxString::Format("%d", port) + "...");
    
    //this makes the socket
    m_socket = new wxSocketClient();
    m_socket->SetEventHandler(*this, SOCKET_ID);
    m_socket->SetNotify(wxSOCKET_CONNECTION_FLAG |
                        wxSOCKET_INPUT_FLAG      |
                        wxSOCKET_LOST_FLAG);
    m_socket->Notify(true);
    
    //address setup
    wxIPV4address addr;
        addr.Hostname(host);
    addr.Service(port);
    
    //non-blocking connect
    m_socket->Connect(addr, false);
    
    //short wait for connection
    if (!m_socket->WaitOnConnect(10)) {
        LogMessage("connect failed");
        wxMessageBox("failed to reach server", "Connection Error", wxICON_ERROR);
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
    SetStatusText("not connected", 1);
    
    m_connectButton->Enable(true);
    m_disconnectButton->Enable(false);
    m_hostInput->Enable(true);
    m_portInput->Enable(true);
        m_messageInput->Enable(false);
    m_sendButton->Enable(false);
    
    LogMessage("disconnected from server");
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

//app bootstrap
class ClientApp : public wxApp {
public:
    virtual bool OnInit();
    virtual int  OnExit();
};

bool ClientApp::OnInit() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        wxMessageBox("winsock init failed", "Error", wxICON_ERROR);
        return false;
    }
#endif

    ClientFrame* frame = new ClientFrame("User2 Chat Client");
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
