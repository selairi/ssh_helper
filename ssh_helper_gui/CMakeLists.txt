set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

option(USE_QT6 "Force use the Qt6." $ENV{USE_QT6})
option(USE_QT5 "Force use the Qt5." $ENV{USE_QT5})

if((USE_QT6 AND USE_QT5) OR
   (NOT USE_QT6 AND NOT USE_QT5))

    # Autodetect Qt version
    find_package(Qt6 QUIET)
    message("-- Searching for Qt6")        
    if(DEFINED Qt6_FOUND)
        set(USE_QT5 OFF)
        set(USE_QT6 ON)
        message("-- Qt6 ${USE_QT6}")
    else()
        set(USE_QT5 ON)
        set(USE_QT6 OFF)
        message("-- Qt5 ${USE_QT5}")
    endif()
endif()


if(USE_QT5)
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
else()
    find_package(Qt6 COMPONENTS Widgets REQUIRED)
endif()

add_executable(ssh_helper_gui
    host.cpp
    main.cpp
    mainwindow.cpp
    mainwindow.ui
    newhost.cpp
    newhost.ui
    script.cpp
    scriptdialog.cpp
    scriptdialog.ui
    monitordialog.cpp
    monitordialog.ui
    guiscriptdialog.cpp
    guiscriptdialog.ui
    copyfiletoclientdialog.ui
    copyfiletoclientdialog.cpp
    ssh.ui
)


set(SHARE_FILES
    share/menu.txt
)

set(APP_ICON_FILES
    desktop_files/ssh_helper.svg
)

# Config file
configure_file ( config.in config.h )
configure_file ( desktop_files/ssh_helper.desktop.in ssh_helper.desktop )

target_include_directories(ssh_helper_gui PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/configfileparser"
)

if(USE_QT5)
  target_link_libraries(ssh_helper_gui 
    PUBLIC ConfigFileParser
    PRIVATE Qt5::Widgets
  )
else()
  target_link_libraries(ssh_helper_gui 
    PUBLIC ConfigFileParser
    PRIVATE Qt6::Widgets
  )
endif()


install(TARGETS ${PROJECT_NAME}_gui RUNTIME DESTINATION bin)
install(FILES ${SHARE_FILES} DESTINATION share/ssh_helper)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ssh_helper.desktop DESTINATION share/applications)
install(FILES ${APP_ICON_FILES} DESTINATION share/icons/hicolor/scalable/apps)

