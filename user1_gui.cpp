// user1_gui.cpp
// simple multi-user chat "server" gui
// user1 = host/server

#include <wx/wx.h>        //this is the wx header file
#include <wx/socket.h>    //sockets header
#include <wx/listctrl.h>  // list for clients
#include <map> 

// windows and linux have different net headers so we need both
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

// client info that is stored for every client connected
struct ClientInfo {
    wxSocketBase* socket;  //their socket
    wxString      address; // they ip:port
    wxString      name;    
    int           id;      //#id
};
//class for the main chat window
class ChatFrame : public wxFrame {
public:
    ChatFrame(const wxString& title, int port);
    ~ChatFrame();

private:
    // ui events handling 
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSendMessage(wxCommandEvent& event);
    void OnServerEvent(wxSocketEvent& event);   // new conn
    void OnSocketEvent(wxSocketEvent& event);   // data / lost
    void OnClientSelected(wxListEvent& event);  // clicked in list
    
    //helpers
    void AcceptConnection();
    void HandleSocketInput(wxSocketBase* sock);
    void BroadcastMessage(const wxString& message);
    void RemoveClient(wxSocketBase* sock);
    void LogMessage(const wxString& message);
    
//these are the bits that show on screen
    wxTextCtrl* m_chatDisplay;
    wxTextCtrl* m_messageInput;
    wxButton*   m_sendButton;
    wxButton*   m_broadcastButton;
    wxListCtrl* m_clientList;
    
//this shows the state of the server when running
    wxSocketServer*                   m_server;
    std::map<wxSocketBase*, ClientInfo> m_clients;
    int   m_nextClientId;
    int   m_port;
    
    wxDECLARE_EVENT_TABLE();  
};

// this is the event id
enum {
    ID_Quit = 1,
    ID_About,
    ID_Send,
    ID_Broadcast,
    ID_ClientList,
    SOCKET_ID,
    SERVER_ID
};
  

//event table for the chat frame documenting which events call which functions
wxBEGIN_EVENT_TABLE(ChatFrame, wxFrame)
    EVT_MENU(ID_Quit,      ChatFrame::OnQuit)
     EVT_MENU(ID_About,     ChatFrame::OnAbout)
    EVT_BUTTON(ID_Send,    ChatFrame::OnSendMessage)
     EVT_BUTTON(ID_Broadcast, ChatFrame::OnSendMessage)
    EVT_SOCKET(SERVER_ID,  ChatFrame::OnServerEvent)
    EVT_SOCKET(SOCKET_ID,  ChatFrame::OnSocketEvent)
     EVT_LIST_ITEM_SELECTED(ID_ClientList, ChatFrame::OnClientSelected)
wxEND_EVENT_TABLE()
   
   //the constructor for the chat frame to help set up the gui
ChatFrame::ChatFrame(const wxString& title, int port)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600)),
      m_nextClientId(1),
      m_port(port)
{
    //menu on the guicd ch
    wxMenu* menuFile = new wxMenu;
      menuFile->Append(ID_About, "&About\tF1", "abt this thing");
    menuFile->AppendSeparator();
     menuFile->Append(ID_Quit, "E&xit\tAlt-X", "close app");
    
    wxMenuBar* menuBar = new wxMenuBar();
    menuBar->Append(menuFile, "&File");
    SetMenuBar(menuBar);
    
    //this makes the status bar at the bottom
    CreateStatusBar(2);
    SetStatusText("chat server up IM sleepy");

    // layout for the gui
    wxPanel*    panel    = new wxPanel(this, wxID_ANY);
     wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // left: clients list
        wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    wxStaticText* clientLabel = new wxStaticText(panel, wxID_ANY, "clients:");
    leftSizer->Add(clientLabel, 0, wxALL, 5);
    
    m_clientList = new wxListCtrl(
        panel,
        ID_ClientList,
        wxDefaultPosition,
        wxSize(200, -1),
        wxLC_REPORT | wxLC_SINGLE_SEL
    );
    m_clientList->AppendColumn("ID", wxLIST_FORMAT_LEFT, 50);
    m_clientList->AppendColumn("Client", wxLIST_FORMAT_LEFT, 140);
    leftSizer->Add(m_clientList, 1, wxALL | wxEXPAND, 5);
    
    mainSizer->Add(leftSizer, 0, wxEXPAND);
    
    // right: chat area
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    m_chatDisplay = new wxTextCtrl(
        panel,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH
    );
    rightSizer->Add(m_chatDisplay, 1, wxALL | wxEXPAND, 5);
    
    //input to type messages 
    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_messageInput = new wxTextCtrl(
        panel,
        wxID_ANY,
        "",
        wxDefaultPosition,
        wxDefaultSize,
        wxTE_PROCESS_ENTER
    );
    m_messageInput->Bind(wxEVT_TEXT_ENTER, &ChatFrame::OnSendMessage, this);
    inputSizer->Add(m_messageInput, 1, wxALL, 5);
    
    m_sendButton = new wxButton(panel, ID_Send, "Send sel");
    inputSizer->Add(m_sendButton, 0, wxALL, 5);
    
    m_broadcastButton = new wxButton(panel, ID_Broadcast, "Send all");
    inputSizer->Add(m_broadcastButton, 0, wxALL, 5);
    
    rightSizer->Add(inputSizer, 0, wxEXPAND);
    mainSizer->Add(rightSizer, 1, wxEXPAND);
    
    panel->SetSizer(mainSizer);
    
    // server socket setup
    wxIPV4address addr;
    addr.Service(port);
    
    m_server = new wxSocketServer(addr);
    
    if (!m_server->Ok()) {
        LogMessage("ERR: cant open srv on port " + wxString::Format("%d", port));
        wxMessageBox("server failed. port busy?", "Error", wxICON_ERROR);
    } else {
        LogMessage("server on port " + wxString::Format("%d", port));
        LogMessage("waiting for clients...");
        SetStatusText(wxString::Format("listening %d", port), 1);
        
        m_server->SetEventHandler(*this, SERVER_ID);
        m_server->SetNotify(wxSOCKET_CONNECTION_FLAG);
        m_server->Notify(true);
    }
}

ChatFrame::~ChatFrame() {
    // drop clients
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
    wxMessageBox(
        "User1 chat server\n\nmulti client chat host.",
        "About",
        wxOK | wxICON_INFORMATION,
        this
    );
}

void ChatFrame::OnSendMessage(wxCommandEvent& event) {
    wxString message = m_messageInput->GetValue().Trim();
    if (message.IsEmpty()) {
        return;
    }

    //this figures out if we are sending to selected client or all
    if (m_clients.empty()) {
        LogMessage("no clients to send to.");
    } else {
        BroadcastMessage("[Server] " + message);
        LogMessage("[Server] " + message);
    }
    m_messageInput->Clear();
}
//accepts new client connections here
void ChatFrame::OnServerEvent(wxSocketEvent& event) {
    if (event.GetSocketEvent() == wxSOCKET_CONNECTION) {
        AcceptConnection();
    }
}
// this is the socket event handler for client sockets
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
    //this gets the selected client from the list and only logs it for now
    long index = event.GetIndex();
    LogMessage("sel: " + m_clientList->GetItemText(index, 1));
}

void ChatFrame::AcceptConnection() {
    wxSocketBase* sock = m_server->Accept(false);
    if (!sock) {
        LogMessage("ERR: accept failed");
        return;
    }
    
    //shows who connected
    wxIPV4address addr;
    sock->GetPeer(addr);
    wxString clientAddr = addr.IPAddress() + ":" + wxString::Format("%d", addr.Service());
    // right here stores in depth client info-------------------
    ClientInfo info;
    info.socket  = sock;
    info.address = clientAddr;
    info.name    = "User" + wxString::Format("%d", m_nextClientId);
    info.id      = m_nextClientId++;
    
    m_clients[sock] = info;
    
    //adds client to the ui list=================
    long index = m_clientList->InsertItem(
        m_clientList->GetItemCount(),
        wxString::Format("%d", info.id)
    );
    m_clientList->SetItem(index, 1, info.name);
    
    LogMessage("client in: " + info.name + " (" + clientAddr + ")");
    SetStatusText(wxString::Format("%d client(s)", (int)m_clients.size()), 1);
    
    //these are the socket events that manage this client--- and not the server socket
    sock->SetEventHandler(*this, SOCKET_ID);
    sock->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
    sock->Notify(true);
    
    //welcome message to the new client
    wxString welcome = "Welcome, " + info.name + "\n";
    sock->Write(welcome.mb_str(), welcome.length());
}
// this handles incoming data from a client socket 
void ChatFrame::HandleSocketInput(wxSocketBase* sock) {
    auto it = m_clients.find(sock);
    if (it == m_clients.end()) {
        return;
    }
    //reads the incoming data
    char buffer[1024];
    sock->Read(buffer, sizeof(buffer) - 1);
    wxUint32 len = sock->LastCount();
    
    if (len == 0) {
        return;
    }
    
    buffer[len] = '\0';
    wxString message(buffer, wxConvUTF8, len);
    message.Trim(true).Trim(false);
    
    if (message.IsEmpty()) {
        return;
    }
    
    // special handling for "Exit" to match project requirements
    if (message == "Exit") {
        LogMessage("[" + it->second.name + "] requested Exit");
        
        // send Exit back so client knows to shut down
        wxString reply = "Exit\n";
        sock->Write(reply.mb_str(), reply.length());
        
        // drop this client cleanly
        RemoveClient(sock);
        return;
    }
    
    // normal chat message: log and broadcast to everyone
    LogMessage("[" + it->second.name + "] " + message);
    BroadcastMessage("[" + it->second.name + "] " + message);
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
    int      id   = it->second.id;
    
    //drop from ui list
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
    
    LogMessage("client out: " + name);
    SetStatusText(wxString::Format("%d client(s)", (int)m_clients.size()), 1);
}

void ChatFrame::LogMessage(const wxString& message) {
    m_chatDisplay->AppendText(message + "\n");
}

//app bootstrap
class ChatApp : public wxApp {
public:
    virtual bool OnInit();
    virtual int  OnExit();
};

bool ChatApp::OnInit() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        wxMessageBox("winsock init fail", "Error", wxICON_ERROR);
        return false;
    }
#endif

    // port from argv or default
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

wxIMPLEMENT_APP(ChatApp);  //macro to implement main()