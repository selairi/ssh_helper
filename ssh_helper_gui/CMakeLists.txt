set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

#find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)

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

# Config file
configure_file ( config.in config.h )

target_include_directories(ssh_helper_gui PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/configfileparser"
)

target_link_libraries(ssh_helper_gui 
  PUBLIC ConfigFileParser
  #PRIVATE Qt6::Widgets
  PRIVATE Qt5::Widgets
)

install(TARGETS ${PROJECT_NAME}_gui RUNTIME DESTINATION bin)
install(FILES ${SHARE_FILES} DESTINATION share/ssh_helper)

