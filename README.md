Multi user chat room on different devices
the server listens for devices trying to connect, then it has tohe option to broadcast messages to users and is also the only one able to see whos all online. User 1& 2 the clients connect to the server by entering the ip and port number then they are able to share messages between each other from different devices. 


user 1-- acts as the server on windows OS
user 2 -- acts as a client gui on raspberryPI OS
user3-optional acts as a client

requirements
visual studio must be installed and c++ compatible
cmake must be installed version 3.10
vcpkg must be setup and - `C:\vcpkg\scripts\buildsystems\vcpkg.cmake` exists  must be there

wINDOWS
mkdir build -ErrorAction SilentlyContinue
cd build
# Configure (if not done yet)
 cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
 cmake --build . --config Release
start .\Release\user1_gui.exe



PI-os
on a separate device -- I used pi- os but tested on Windows using the same above steps
#1 Navigate to the project folder
cd chat-room-mtsu
mkdir -p build
cd build
cmake .. -DCMAKE_BUILE_TYPE=Release
cmake --build .
 #3 run the user two client using ./user2_gui
#4 in the user's two chat window, insert the IP of user one (mine =10.0.0.101) and the port # 8888
#5 Click Connect

