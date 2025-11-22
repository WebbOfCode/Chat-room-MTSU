# Chat-room-MTSU
Multi-User TCP Chat Room with GUI

## Overview
This project implements a multi-user chat system with graphical user interfaces. User1 acts as a server that can accept multiple simultaneous client connections (user2, user3, etc.), enabling real-time group chat functionality.

## Features
- **GUI-based Interface**: Modern graphical interface using wxWidgets
- **Multi-User Support**: User1 can communicate with multiple clients simultaneously
- **Real-time Messaging**: Instant message delivery between connected users
- **Selective/Broadcast Messaging**: User1 can send messages to specific clients or broadcast to all
- **Connection Management**: Visual display of connected clients with connection/disconnection notifications

## Requirements

### Windows
- Visual Studio 2019 or later (with C++ desktop development workload)
- CMake 3.10 or later
- wxWidgets 3.1 or later

### Linux/macOS
- GCC 7+ or Clang 5+
- CMake 3.10 or later
- wxWidgets 3.1 or later
- Development libraries: `libwxgtk3.0-gtk3-dev` (Ubuntu/Debian) or equivalent

## Installing wxWidgets

### Windows
1. Download wxWidgets from https://www.wxwidgets.org/downloads/
2. Extract to a directory (e.g., `C:\wxWidgets`)
3. Build wxWidgets:
   ```
   cd C:\wxWidgets\build\msw
   nmake /f makefile.vc BUILD=release
   ```
4. Set environment variable:
   ```
   set wxWidgets_ROOT_DIR=C:\wxWidgets
   ```

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libwxgtk3.0-gtk3-dev
```

### macOS
```bash
brew install wxwidgets
```

## Building the Project

### Using CMake (Recommended)

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Executables will be in build/Release/ (Windows) or build/ (Unix)
```

### Windows (Visual Studio)
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

## Running the Application

### Step 1: Start the Server (User1)
Run `user1_gui` first. It will start listening for connections on port 8888 by default.

**Windows:**
```
.\build\Release\user1_gui.exe
```

**Linux/macOS:**
```
./build/user1_gui
```

Optionally specify a different port:
```
.\user1_gui.exe 9000
```

### Step 2: Connect Clients (User2, User3, etc.)
Run multiple client instances. Each will connect to User1.

**Windows:**
```
.\build\Release\user2_gui.exe
.\build\Release\user3_gui.exe
```

**Linux/macOS:**
```
./build/user2_gui &
./build/user3_gui &
```

In each client:
1. Enter the server address (default: 127.0.0.1 for local testing)
2. Enter the port (default: 8888)
3. Click "Connect"
4. Start chatting!

## Usage Guide

### User1 (Server)
- **Connected Clients List**: Shows all connected users with unique IDs
- **Select Client**: Click on a client in the list to send them a direct message
- **Send to Selected**: Sends message only to the selected client
- **Broadcast to All**: Sends message to all connected clients
- **Press Enter**: Broadcasts message by default

### User2/User3 (Clients)
- **Connection Panel**: Enter server IP and port, then click "Connect"
- **Message Input**: Type your message and press Enter or click "Send"
- **Chat Display**: Shows all messages from the server and your sent messages
- **Disconnect**: Close connection using the "Disconnect" button

## Architecture

### Components
- **user1_gui.cpp**: Multi-client server with GUI
  - Accepts multiple simultaneous connections
  - Manages client list
  - Supports selective and broadcast messaging
  
- **user2_gui.cpp**: Client application with GUI
  - Connects to user1 server
  - Sends/receives messages in real-time
  
- **user3_gui.cpp**: Additional client (identical to user2)
  - Allows testing multi-user functionality

### Protocol
- TCP socket communication
- Simple text-based message protocol
- Newline-delimited messages
- Automatic connection handling

## Troubleshooting

### "Could not create server socket"
- Port is already in use. Try a different port number.
- On Windows, ensure Windows Firewall allows the application.
- On Linux, ports below 1024 require sudo.

### "Failed to connect to server"
- Verify user1_gui is running
- Check IP address and port are correct
- Ensure firewall allows connections
- For remote connections, use the actual IP address (not 127.0.0.1)

### Build errors
- Ensure wxWidgets is properly installed
- Verify CMake can find wxWidgets: `cmake .. -DwxWidgets_ROOT_DIR=/path/to/wxWidgets`
- Check that C++17 compiler support is available

## Original Command-Line Versions
The repository still contains the original command-line implementations (`user1.cpp` and `user2.cpp`) which use the alternating turn-based protocol. These are built automatically on Unix-like systems but are superseded by the GUI versions for better usability.

## License
Educational project for MTSU students.

## Contributing
This is a course project. For improvements or bug fixes, please contact the repository maintainer.
