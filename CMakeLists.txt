cmake_minimum_required(VERSION 3.10)
project(ChatRoomMTSU)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base net)
include(${wxWidgets_USE_FILE})

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# User1 GUI Server
add_executable(user1_gui WIN32 user1_gui.cpp)
target_link_libraries(user1_gui ${wxWidgets_LIBRARIES})
if(WIN32)
    target_link_libraries(user1_gui ws2_32)
endif()

# User2 GUI Client
add_executable(user2_gui WIN32 user2_gui.cpp)
target_link_libraries(user2_gui ${wxWidgets_LIBRARIES})
if(WIN32)
    target_link_libraries(user2_gui ws2_32)
endif()

# User3 GUI Client
add_executable(user3_gui WIN32 user3_gui.cpp)
target_link_libraries(user3_gui ${wxWidgets_LIBRARIES})
if(WIN32)
    target_link_libraries(user3_gui ws2_32)
endif()

# Optional: Build original command-line versions (Unix-like systems only)
if(UNIX)
    add_executable(user1 user1.cpp)
    add_executable(user2 user2.cpp)
endif()

# Installation
install(TARGETS user1_gui user2_gui user3_gui
        RUNTIME DESTINATION bin)

# Print build information
message(STATUS "Build configuration:")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  wxWidgets version: ${wxWidgets_VERSION_STRING}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
