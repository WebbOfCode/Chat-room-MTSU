// user1_gui.cpp
// Multi-client GUI chat server.
// Simplified: minimal includes & human-readable comments.

#include <wx/wx.h>
#include <wx/socket.h>
#include <wx/listctrl.h>
#include <map>
// Minimal broadcast server; remove per-client sending logic.
// No global using-directive for std; qualify explicitly.

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

// Client information structure
struct ClientInfo {
    wxSocketBase* socket;
    wxString address;
    wxString name;
    int id;
};

class ChatFrame : public wxFrame {
public:
    ChatFrame(const wxString& title, int port);
    ~ChatFrame();

private:
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSendMessage(wxCommandEvent& event);
    void OnServerEvent(wxSocketEvent& event);
    void OnSocketEvent(wxSocketEvent& event);
    void OnClientSelected(wxListEvent& event);
    
    void AcceptConnection();
    void HandleSocketInput(wxSocketBase* sock);
    void BroadcastMessage(const wxString& message);
    void RemoveClient(wxSocketBase* sock);
    void LogMessage(const wxString& message);
    
    wxTextCtrl* m_chatDisplay;
    wxTextCtrl* m_messageInput;
    wxButton* m_sendButton;
    wxButton* m_broadcastButton;
    wxListCtrl* m_clientList;
    
    wxSocketServer* m_server;
    std::map<wxSocketBase*, ClientInfo> m_clients; // active clients keyed by socket
    int m_nextClientId;
    int m_port;
    
    wxDECLARE_EVENT_TABLE();
};

enum {
    ID_Quit = 1,
    ID_About,
    ID_Send,
    ID_Broadcast,
    ID_ClientList,
    SOCKET_ID,
    SERVER_ID
};

wxBEGIN_EVENT_TABLE(ChatFrame, wxFrame)
    EVT_MENU(ID_Quit, ChatFrame::OnQuit)
    EVT_MENU(ID_About, ChatFrame::OnAbout)
    EVT_BUTTON(ID_Send, ChatFrame::OnSendMessage)
    EVT_BUTTON(ID_Broadcast, ChatFrame::OnSendMessage)
    EVT_SOCKET(SERVER_ID, ChatFrame::OnServerEvent)
    EVT_SOCKET(SOCKET_ID, ChatFrame::OnSocketEvent)
    EVT_LIST_ITEM_SELECTED(ID_ClientList, ChatFrame::OnClientSelected)
wxEND_EVENT_TABLE()

ChatFrame::ChatFrame(const wxString& title, int port)
        : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
            m_nextClientId(1), m_port(port) {
    
    // Create menu bar
    wxMenu* menuFile = new wxMenu;
    menuFile->Append(ID_About, "&About\tF1", "Show about dialog");
    menuFile->AppendSeparator();
    menuFile->Append(ID_Quit, "E&xit\tAlt-X", "Quit this program");
    
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);
    
    CreateStatusBar(2);
    SetStatusText("Welcome to Chat Server!");
    
    // Create main panel
    wxPanel* panel = new wxPanel(this, wxID_ANY);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left side - Client list
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* clientLabel = new wxStaticText(panel, wxID_ANY, "Connected Clients:");
    leftSizer->Add(clientLabel, 0, wxALL, 5);
    
    m_clientList = new wxListCtrl(panel, ID_ClientList, wxDefaultPosition, wxSize(200, -1),
                                    wxLC_REPORT | wxLC_SINGLE_SEL);
    m_clientList->AppendColumn("ID", wxLIST_FORMAT_LEFT, 50);
    m_clientList->AppendColumn("Client", wxLIST_FORMAT_LEFT, 140);
    leftSizer->Add(m_clientList, 1, wxALL | wxEXPAND, 5);
    
    mainSizer->Add(leftSizer, 0, wxEXPAND);
    
    // Right side - Chat area
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    // Chat display
    m_chatDisplay = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    rightSizer->Add(m_chatDisplay, 1, wxALL | wxEXPAND, 5);
    
    // Message input area
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_messageInput = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                     wxTE_PROCESS_ENTER);
    m_messageInput->Bind(wxEVT_TEXT_ENTER, &ChatFrame::OnSendMessage, this);
    inputSizer->Add(m_messageInput, 1, wxALL, 5);
    
    m_sendButton = new wxButton(panel, ID_Send, "Send to Selected");
    inputSizer->Add(m_sendButton, 0, wxALL, 5);
    
    m_broadcastButton = new wxButton(panel, ID_Broadcast, "Broadcast to All");
    inputSizer->Add(m_broadcastButton, 0, wxALL, 5);
    
    rightSizer->Add(inputSizer, 0, wxEXPAND);
    mainSizer->Add(rightSizer, 1, wxEXPAND);
    
    panel->SetSizer(mainSizer);
    
    // Create socket server
    wxIPV4address addr;
    addr.Service(port);
    
    m_server = new wxSocketServer(addr);
    
    if (!m_server->Ok()) {
        LogMessage("ERROR: Could not create server socket on port " + wxString::Format("%d", port));
        wxMessageBox("Failed to create server. Port may be in use.", "Error", wxICON_ERROR);
    } else {
        LogMessage("Server started on port " + wxString::Format("%d", port));
        LogMessage("Waiting for clients to connect...");
        SetStatusText(wxString::Format("Server listening on port %d", port), 1);
        
        m_server->SetEventHandler(*this, SERVER_ID);
        m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
        m_server->Notify(true);
    }
}

ChatFrame::~ChatFrame() {
    // Clean up all client connections
    for (auto& pair : m_clients) {
        pair.first->Destroy();
    }
    m_clients.clear();
    
    if (m_server) {
        m_server->Destroy();
    }
}

void ChatFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    Close(true);
}

void ChatFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    wxMessageBox("Multi-User Chat Server (User1)\n\n"
                 "Supports multiple simultaneous client connections.\n"
                 "Clients can be user2, user3, etc.",
                 "About User1 Chat Server",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void ChatFrame::OnSendMessage(wxCommandEvent& event) {
    wxString message = m_messageInput->GetValue().Trim();
    if (message.IsEmpty()) {
        return;
    }
    // Always broadcast regardless of which button was pressed or Enter.
    if (m_clients.empty()) {
        LogMessage("No clients connected to send message to.");
    } else {
        BroadcastMessage("[Server] " + message);
        LogMessage("[Server] " + message);
    }
    m_messageInput->Clear();
}

void ChatFrame::OnServerEvent(wxSocketEvent& event) {
    if (event.GetSocketEvent() == wxSOCKET_CONNECTION) {
        AcceptConnection();
    }
}

void ChatFrame::OnSocketEvent(wxSocketEvent& event) {
    wxSocketBase* sock = event.GetSocket();
    
    switch (event.GetSocketEvent()) {
        case wxSOCKET_INPUT:
            HandleSocketInput(sock);
            break;
            
        case wxSOCKET_LOST:
            RemoveClient(sock);
            break;
            
        default:
            break;
    }
}

void ChatFrame::OnClientSelected(wxListEvent& event) {
    // Selection retained for UI consistency; no per-client send.
    long index = event.GetIndex();
    LogMessage("Selected: " + m_clientList->GetItemText(index, 1));
}

void ChatFrame::AcceptConnection() {
    wxSocketBase* sock = m_server->Accept(false);
    if (!sock) {
        LogMessage("ERROR: Failed to accept connection");
        return;
    }
    
    // Get client address
    wxIPV4address addr;
    sock->GetPeer(addr);
    wxString clientAddr = addr.IPAddress() + ":" + wxString::Format("%d", addr.Service());
    
    ClientInfo info;
    info.socket = sock;
    info.address = clientAddr;
    info.name = "User" + wxString::Format("%d", m_nextClientId);
    info.id = m_nextClientId++;
    
    m_clients[sock] = info;
    
    // Add to list
    long index = m_clientList->InsertItem(m_clientList->GetItemCount(), wxString::Format("%d", info.id));
    m_clientList->SetItem(index, 1, info.name);
    
    LogMessage("Client connected: " + info.name + " (" + clientAddr + ")");
    SetStatusText(wxString::Format("%d client(s) connected", (int)m_clients.size()), 1);
    
    // Setup event handlers for this socket
    sock->SetEventHandler(*this, SOCKET_ID);
    sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    sock->Notify(true);
    
    // Send welcome message
    wxString welcome = "Welcome! You are connected as " + info.name + "\n";
    sock->Write(welcome.mb_str(), welcome.length());
}

void ChatFrame::HandleSocketInput(wxSocketBase* sock) {
    auto it = m_clients.find(sock);
    if (it == m_clients.end()) {
        return;
    }
    
    char buffer[1024];
    sock->Read(buffer, sizeof(buffer) - 1);
    wxUint32 len = sock->LastCount();
    
    if (len > 0) {
        buffer[len] = '\0';
        wxString message(buffer, wxConvUTF8, len);
        message.Trim();

        if (!message.IsEmpty()) {
            // Log locally
            LogMessage("[" + it->second.name + "] " + message);
            // Broadcast to all clients (including sender)
            BroadcastMessage("[" + it->second.name + "] " + message);
        }
    }
}


void ChatFrame::BroadcastMessage(const wxString& message) {
    wxString msg = message + "\n";
    for (auto& pair : m_clients) {
        pair.first->Write(msg.mb_str(), msg.length());
    }
}

void ChatFrame::RemoveClient(wxSocketBase* sock) {
    auto it = m_clients.find(sock);
    if (it == m_clients.end()) {
        return;
    }
    
    wxString name = it->second.name;
    int id = it->second.id;
    
    // Remove from list control
    for (int i = 0; i < m_clientList->GetItemCount(); i++) {
        wxString idStr = m_clientList->GetItemText(i, 0);
        long itemId;
        if (idStr.ToLong(&itemId) && itemId == id) {
            m_clientList->DeleteItem(i);
            break;
        }
    }
    
    sock->Destroy();
    m_clients.erase(it);
    
    LogMessage("Client disconnected: " + name);
    SetStatusText(wxString::Format("%d client(s) connected", (int)m_clients.size()), 1);
    
}

void ChatFrame::LogMessage(const wxString& message) {
    m_chatDisplay->AppendText(message + "\n");
}

// Application class
class ChatApp : public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();
};

bool ChatApp::OnInit() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        wxMessageBox("Failed to initialize Winsock", "Error", wxICON_ERROR);
        return false;
    }
#endif

    // Get port from command line or use default
    int port = 8888;
    if (argc > 1) {
        long p;
        wxString(argv[1]).ToLong(&p);
        if (p > 0 && p < 65536) {
            port = p;
        }
    }
    
    ChatFrame* frame = new ChatFrame("User1 Chat Server", port);
    frame->Show(true);
    return true;
}

int ChatApp::OnExit() {
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

wxIMPLEMENT_APP(ChatApp);
