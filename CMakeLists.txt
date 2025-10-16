cmake_minimum_required(VERSION 3.16)

project(GUITaskManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 🎯 This is the section you need to modify.
# Add your backend .cpp files to this list.
set(PROJECT_SOURCES
        main.cpp          # This is the GUI's main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        # --- Add your backend files here ---
        processParser.cpp
        ServiceManager.cpp
        StartupManager.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GUITaskManager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(GUITaskManager
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(GUITaskManager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# This section is for macOS/iOS/Windows/install rules and can be left as is.
set_target_properties(GUITaskManager PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.GUITaskManager
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS GUITaskManager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GUITaskManager)
endif()