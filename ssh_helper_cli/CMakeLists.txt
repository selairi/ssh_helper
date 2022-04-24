include(FindPkgConfig)

pkg_check_modules(LIBSSH REQUIRED libssh>=0.9)
pkg_check_modules(LIBSSL REQUIRED libssl>=1.1)

add_executable(ssh_helper_cli
  clientthread.cpp
  main.cpp
  manager.cpp
  p2pdata.cpp
  sshptr.cpp
  threadshareddata.cpp
)

target_link_libraries(ssh_helper_cli 
  ConfigFileParser
  ${LIBSSH_LIBRARIES} 
  ${LIBSSL_LIBRARIES}
  pthread
)

target_include_directories(ssh_helper_cli PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/configfileparser"
)

install(TARGETS ${PROJECT_NAME}_cli RUNTIME DESTINATION bin)
